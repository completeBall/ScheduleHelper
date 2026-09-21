import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

Dialog {
    id: dlg
    objectName: "settingsDialog"

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: Math.min(560, parent.width - 48)
    padding: 28
    background: Rectangle { radius: Theme.radiusLg; color: Theme.surface; border.color: Theme.border }
    Overlay.modal: Rectangle { color: Theme.scrim }

    contentItem: ColumnLayout {
        spacing: 18

        Text {
            text: "设置与说明"
            font.family: Theme.fontUi
            font.pixelSize: Theme.textXl
            font.weight: Font.DemiBold
            color: Theme.text
        }

        FormField {
            Layout.fillWidth: true
            label: "外观"
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: [{id: "system", t: "跟随系统", i: Theme.icon.settings},
                            {id: "light", t: "浅色", i: Theme.icon.sun},
                            {id: "dark", t: "深色", i: Theme.icon.moon}]
                    delegate: AppButton {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.t
                        glyph: modelData.i
                        variant: app.themeMode === modelData.id ? "primary" : "secondary"
                        onClicked: app.themeMode = modelData.id
                    }
                }
            }
        }

        FormField {
            Layout.fillWidth: true
            label: "数据位置"
            hint: "活动缓存、课表和登录状态都保存在这里；只读取活动信息，不会自动报名。"
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    text: app.dataDir
                    elide: Text.ElideMiddle
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textSm
                    color: Theme.text
                }
                AppButton {
                    text: "打开文件夹"
                    glyph: Theme.icon.folder
                    onClicked: app.openDataFolder()
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: "使用说明"
                font.family: Theme.fontUi
                font.pixelSize: Theme.textMd
                font.weight: Font.DemiBold
                color: Theme.text
            }
            Repeater {
                model: [
                    "1. 打开「学校登录」，在程序内完成学校统一认证。",
                    "2. 回到「活动汇总」点击「开始采集」，自动读取全部未开始的活动。",
                    "3. 搜索、筛选后可「导出 Excel」；团日、班会和班级活动会自动排除。",
                    "4. 在「我的课表」登录教务系统并「一键导入课表」，再「设置日期」，即可在课表里看到橙色报名提醒和蓝色活动开始提醒。",
                    "登录状态过期时重新登录即可。"
                ]
                delegate: Text {
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    wrapMode: Text.Wrap
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textSm
                    color: Theme.textMuted
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            AppButton { text: "完成"; variant: "primary"; onClicked: dlg.close() }
        }
    }
}
