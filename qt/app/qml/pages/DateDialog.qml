import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

Dialog {
    id: dlg
    objectName: "dateDialog"
    property string error: ""

    function openDialog() {
        error = ""
        weekSpin.value = app.schedule.week > 0 ? app.schedule.week : 1
        dayCombo.currentIndex = 0
        const existing = app.schedule.hasAnchor ? app.schedule.dateOf(weekSpin.value, 1) : ""
        dateField.text = existing !== "" ? existing : app.dateToday()
        open()
        dateField.forceActiveFocus()
    }

    function save() {
        const err = app.schedule.setAnchor(weekSpin.value, dayCombo.currentIndex + 1, dateField.text.trim())
        if (err !== "") {
            error = err
            return
        }
        close()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: 440
    padding: 24
    background: Rectangle { radius: Theme.radiusLg; color: Theme.surface; border.color: Theme.border }
    Overlay.modal: Rectangle { color: Theme.scrim }

    contentItem: ColumnLayout {
        spacing: 14

        Text {
            text: "设置课表日期"
            font.family: Theme.fontUi
            font.pixelSize: Theme.textXl
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            Layout.fillWidth: true
            text: "只需告诉我任意一周的任意一天是几号，其他周和日期会自动补全。"
            wrapMode: Text.Wrap
            font.family: Theme.fontUi
            font.pixelSize: Theme.textSm
            color: Theme.textMuted
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            FormField {
                label: "第几周"
                Layout.fillWidth: true
                SpinBox {
                    id: weekSpin
                    Layout.fillWidth: true
                    from: 1
                    to: 40
                    editable: true
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textMd
                }
            }
            FormField {
                label: "星期"
                Layout.fillWidth: true
                FilterCombo {
                    id: dayCombo
                    Layout.fillWidth: true
                    model: app.schedule.dayNames
                }
            }
        }

        FormField {
            Layout.fillWidth: true
            label: "这一天的日期"
            error: dlg.error
            hint: "格式：2026-09-23"
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: dateField
                    Layout.fillWidth: true
                    implicitHeight: 38
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textMd
                    color: Theme.text
                    selectByMouse: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    placeholderText: "yyyy-mm-dd"
                    placeholderTextColor: Theme.textFaint
                    onAccepted: dlg.save()
                    onTextEdited: dlg.error = ""
                    background: Rectangle {
                        radius: Theme.radiusSm + 2
                        color: Theme.surface
                        border.width: dateField.activeFocus ? 2 : 1
                        border.color: dlg.error !== "" ? Theme.danger : dateField.activeFocus ? Theme.primary : Theme.border
                    }
                }
                AppButton {
                    text: "今天"
                    onClicked: dateField.text = app.dateToday()
                }
            }
        }

        RowLayout {
            Layout.topMargin: 6
            Layout.alignment: Qt.AlignRight
            spacing: 10
            AppButton { text: "取消"; onClicked: dlg.close() }
            AppButton { text: "保存日期"; variant: "primary"; onClicked: dlg.save() }
        }
    }
}
