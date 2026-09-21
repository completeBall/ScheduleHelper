import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtWebEngine
import Gdipu

// Both embedded browsers live here for the whole session so login state and the
// scraper's pages survive while other pages are showing.
Rectangle {
    id: page
    color: Theme.bg

    property WebEngineProfile profile: null
    readonly property var panes: [schoolPane, academicPane]
    readonly property var current: panes[app.browserTab]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- toolbar ---------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: Theme.surface

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 10

                // segmented switch
                Rectangle {
                    Layout.preferredWidth: seg.implicitWidth + 8
                    Layout.preferredHeight: 40
                    radius: Theme.radius
                    color: Theme.surfaceAlt
                    Row {
                        id: seg
                        anchors.centerIn: parent
                        spacing: 4
                        Repeater {
                            model: [{t: "学校活动系统", i: Theme.icon.school}, {t: "教务系统", i: Theme.icon.calendar}]
                            delegate: AppButton {
                                required property var modelData
                                required property int index
                                text: modelData.t
                                glyph: modelData.i
                                implicitHeight: 32
                                variant: app.browserTab === index ? "secondary" : "ghost"
                                onClicked: app.browserTab = index
                            }
                        }
                    }
                }

                AppButton {
                    glyph: Theme.icon.back
                    variant: "ghost"
                    implicitWidth: 38
                    leftPadding: 0
                    rightPadding: 0
                    onClicked: page.current.goBack()
                }
                AppButton {
                    glyph: Theme.icon.refresh
                    variant: "ghost"
                    implicitWidth: 38
                    leftPadding: 0
                    rightPadding: 0
                    onClicked: page.current.reload()
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: Theme.radiusSm + 2
                    color: Theme.surfaceAlt
                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideMiddle
                        text: page.current.address !== "" ? page.current.address : "尚未打开页面"
                        font.family: Theme.fontUi
                        font.pixelSize: Theme.textSm
                        color: Theme.textMuted
                    }
                }

                AppButton {
                    text: app.browserTab === 0 ? "打开活动广场" : "打开教务首页"
                    glyph: Theme.icon.globe
                    enabled: !app.scraper.busy && !app.importer.busy
                    onClicked: app.browserTab === 0 ? app.openSchool() : app.openAcademic()
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.border
            }
            Rectangle {
                visible: page.current.loading
                anchors.bottom: parent.bottom
                width: parent.width * page.current.loadProgress / 100
                height: 2
                color: Theme.primary
            }
        }

        // ---- hint ------------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.primarySoft
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 8
                FluentIcon { glyph: Theme.icon.info; size: 14; color: Theme.primary }
                Text {
                    Layout.fillWidth: true
                    text: app.browserTab === 0
                          ? "在这里完成学校统一认证登录；登录状态会保存在本机。之后回到「活动汇总」开始采集。"
                          : "在这里登录教务系统；登录后回到「我的课表」点击「一键导入课表」。"
                    elide: Text.ElideRight
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textSm
                    color: Theme.text
                }
            }
        }

        // ---- the two browsers ---------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            WebPane {
                id: schoolPane
                anchors.fill: parent
                bridge: app.schoolBridge
                profile: page.profile
                opacity: app.browserTab === 0 ? 1 : 0
                enabled: app.browserTab === 0
            }
            WebPane {
                id: academicPane
                anchors.fill: parent
                bridge: app.academicBridge
                profile: page.profile
                opacity: app.browserTab === 1 ? 1 : 0
                enabled: app.browserTab === 1
            }

            EmptyState {
                anchors.centerIn: parent
                visible: page.current.address === "" && !page.current.loading
                icon: Theme.icon.globe
                title: "还没有打开页面"
                subtitle: app.browserTab === 0
                          ? "打开学校活动广场并在这里完成统一认证登录。登录状态会保存在本机，之后不必重复登录。"
                          : "打开教务系统首页并在这里登录。登录后回到「我的课表」点击「一键导入课表」。"
                AppButton {
                    text: app.browserTab === 0 ? "打开活动广场" : "打开教务首页"
                    glyph: Theme.icon.globe
                    variant: "primary"
                    onClicked: app.browserTab === 0 ? app.openSchool() : app.openAcademic()
                }
            }

            // keep hands off the page while a script drives it
            Rectangle {
                anchors.fill: parent
                visible: (app.browserTab === 0 && app.scraper.busy) || (app.browserTab === 1 && app.importer.busy)
                color: Theme.scrim
                MouseArea { anchors.fill: parent; hoverEnabled: true }
                Card {
                    anchors.centerIn: parent
                    padding: 24
                    implicitWidth: busyText.implicitWidth + 48
                    implicitHeight: busyText.implicitHeight + 48
                    Text {
                        id: busyText
                        text: app.browserTab === 0 ? "正在自动读取活动，请勿操作此页面…" : "正在读取教务课表，请稍候…"
                        font.family: Theme.fontUi
                        font.pixelSize: Theme.textMd
                        color: Theme.text
                    }
                }
            }
        }
    }
}
