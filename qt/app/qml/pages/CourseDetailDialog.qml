import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

// Full text of a course or reminder card (cells only show a few lines).
Dialog {
    id: dlg
    property var info: ({})

    function openFor(card) {
        info = card
        open()
    }

    readonly property color accent: info.kind === "registration" ? Theme.reminderReg
                                  : info.kind === "event" ? Theme.reminderEvt
                                  : Theme.course(info.color || 0)[1]

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: 420
    padding: 24
    background: Rectangle { radius: Theme.radiusLg; color: Theme.surface; border.color: Theme.border }
    Overlay.modal: Rectangle { color: Theme.scrim }

    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            spacing: 10
            Rectangle { Layout.preferredWidth: 4; Layout.preferredHeight: 22; radius: 2; color: dlg.accent }
            Text {
                Layout.fillWidth: true
                text: dlg.info.dialogTitle || dlg.info.title || ""
                wrapMode: Text.Wrap
                font.family: Theme.fontUi
                font.pixelSize: Theme.textXl
                font.weight: Font.DemiBold
                color: Theme.text
            }
        }
        Text {
            Layout.fillWidth: true
            visible: dlg.info.kind === "course"
            text: {
                const full = dlg.info.full || ""
                const title = dlg.info.title || ""
                return full.indexOf(title) === 0 ? full.substring(title.length).trim() : full
            }
            wrapMode: Text.Wrap
            lineHeight: 1.3
            font.family: Theme.fontUi
            font.pixelSize: Theme.textMd
            color: Theme.text
        }
        Text {
            Layout.fillWidth: true
            visible: dlg.info.kind !== "course"
            text: dlg.info.full || ""
            wrapMode: Text.Wrap
            lineHeight: 1.3
            font.family: Theme.fontUi
            font.pixelSize: Theme.textMd
            color: Theme.text
        }
        Text {
            visible: dlg.info.manual === true
            text: "这是手动添加的课程，可在卡片上点 ✕ 删除。"
            font.family: Theme.fontUi
            font.pixelSize: Theme.textXs
            color: Theme.textMuted
        }

        RowLayout {
            Layout.topMargin: 6
            Layout.alignment: Qt.AlignRight
            spacing: 10
            AppButton {
                visible: (dlg.info.url || "") !== ""
                text: "查看活动详情"
                glyph: Theme.icon.openWindow
                onClicked: {
                    dlg.close()
                    app.openActivity(dlg.info.url)
                }
            }
            AppButton { text: "关闭"; variant: "primary"; onClicked: dlg.close() }
        }
    }
}
