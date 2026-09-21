import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtWebEngine
import Gdipu

ApplicationWindow {
    id: win
    width: 1280
    height: 820
    minimumWidth: 900
    minimumHeight: 620
    visible: true
    title: "广轻活动汇总"
    color: Theme.bg

    readonly property bool compact: width < 1140

    Binding { target: Theme; property: "mode"; value: app.themeMode }

    // one persistent profile for both sites: keeps the school / academic logins
    WebEngineProfile {
        id: profile
        storageName: "GdipuActivityHelper"
        persistentStoragePath: app.profilePath
        offTheRecord: app.testing
        persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        httpAcceptLanguage: "zh-CN,zh;q=0.9,en;q=0.6"
        // Present as plain Edge (same Chromium 122 as the engine): the default "QtWebEngine/x.y"
        // token is something campus firewalls sometimes reject with "403 Request forbidden".
        httpUserAgent: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
                       + "Chrome/122.0.0.0 Safari/537.36 Edg/122.0.0.0"
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- sidebar --------------------------------------------------------------------
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: win.compact ? 76 : 224
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 22
                anchors.bottomMargin: 16
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6

                // brand
                RowLayout {
                    Layout.fillWidth: true
                    Layout.bottomMargin: 22
                    Layout.leftMargin: win.compact ? 0 : 6
                    spacing: 12
                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: win.compact
                        Layout.preferredWidth: 40
                        Layout.maximumWidth: 40
                        Layout.preferredHeight: 40
                        radius: 12
                        color: "#1fb3a5"
                        Text {
                            anchors.centerIn: parent
                            text: "广"
                            font.family: Theme.fontUi
                            font.pixelSize: 22
                            font.weight: Font.Bold
                            color: "#06302d"
                        }
                    }
                    ColumnLayout {
                        visible: !win.compact
                        spacing: 0
                        Text {
                            text: "广轻活动汇总"
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.textLg
                            font.weight: Font.DemiBold
                            color: Theme.sidebarTextActive
                        }
                        Text {
                            text: "GDIPU · 校园活动"
                            font.family: Theme.fontUi
                            font.pixelSize: 11
                            font.letterSpacing: 1
                            color: "#7fb5ae"
                        }
                    }
                }

                NavButton {
                    Layout.fillWidth: true
                    text: "活动汇总"
                    glyph: Theme.icon.list
                    compact: win.compact
                    active: app.page === 0
                    busy: app.scraper.busy
                    onClicked: app.page = 0
                }
                NavButton {
                    Layout.fillWidth: true
                    text: "我的课表"
                    glyph: Theme.icon.calendar
                    compact: win.compact
                    active: app.page === 1
                    busy: app.importer.busy
                    onClicked: app.page = 1
                }
                NavButton {
                    Layout.fillWidth: true
                    text: "学校 / 教务登录"
                    glyph: Theme.icon.globe
                    compact: win.compact
                    active: app.page === 2
                    onClicked: app.page = 2
                }

                Item { Layout.fillHeight: true }

                NavButton {
                    Layout.fillWidth: true
                    text: "设置与说明"
                    glyph: Theme.icon.settings
                    compact: win.compact
                    onClicked: settingsDialog.open()
                }
            }
        }

        // ---- content ---------------------------------------------------------------------
        // Pages are stacked (not hidden by a StackLayout) so the browser underneath stays
        // visible to Chromium and is never throttled while the scraper is driving it.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            BrowserPage {
                anchors.fill: parent
                profile: profile
            }
            ActivitiesPage {
                anchors.fill: parent
                visible: app.page === 0
            }
            SchedulePage {
                anchors.fill: parent
                visible: app.page === 1
            }
        }
    }

    Toast { id: toast }
    Connections {
        target: app
        function onToast(message, kind) { toast.show(message, kind) }
    }

    SettingsDialog { id: settingsDialog }
}
