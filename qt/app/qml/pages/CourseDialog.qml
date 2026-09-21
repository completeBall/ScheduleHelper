import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

Dialog {
    id: dlg
    objectName: "courseDialog"
    property string nameError: ""
    property string weeksError: ""

    function openDialog() {
        nameError = ""
        weeksError = ""
        nameField.text = ""
        teacherField.text = ""
        placeField.text = ""
        dayCombo.currentIndex = 0
        slotCombo.currentIndex = 0
        weeksField.text = app.schedule.week > 0 ? String(app.schedule.week) : "1"
        open()
        nameField.forceActiveFocus()
    }

    function save() {
        nameError = ""
        weeksError = ""
        const err = app.schedule.addManualCourse(nameField.text, teacherField.text, placeField.text,
                                                 dayCombo.currentIndex + 1, slotCombo.currentIndex, weeksField.text)
        if (err === "") {
            close()
        } else if (err.indexOf("课程名称") >= 0) {
            nameError = err
        } else {
            weeksError = err
        }
    }

    component Input: TextField {
        property bool invalid: false
        Layout.fillWidth: true
        implicitHeight: 38
        font.family: Theme.fontUi
        font.pixelSize: Theme.textMd
        color: Theme.text
        selectByMouse: true
        placeholderTextColor: Theme.textFaint
        onAccepted: dlg.save()
        background: Rectangle {
            radius: Theme.radiusSm + 2
            color: Theme.surface
            border.width: parent.activeFocus ? 2 : 1
            border.color: parent.invalid ? Theme.danger : parent.activeFocus ? Theme.primary : Theme.border
        }
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: 500
    padding: 24
    background: Rectangle { radius: Theme.radiusLg; color: Theme.surface; border.color: Theme.border }
    Overlay.modal: Rectangle { color: Theme.scrim }

    contentItem: ColumnLayout {
        spacing: 14

        Text {
            text: "新增课程"
            font.family: Theme.fontUi
            font.pixelSize: Theme.textXl
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            Layout.fillWidth: true
            text: "用于补充临时调课。手动课程会在重新导入教务课表时保留。"
            wrapMode: Text.Wrap
            font.family: Theme.fontUi
            font.pixelSize: Theme.textSm
            color: Theme.textMuted
        }

        FormField {
            Layout.fillWidth: true
            label: "课程名称"
            error: dlg.nameError
            Input {
                id: nameField
                maximumLength: 80
                invalid: dlg.nameError !== ""
                onTextEdited: dlg.nameError = ""
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            FormField {
                Layout.fillWidth: true
                label: "教师"
                Input { id: teacherField; maximumLength: 50 }
            }
            FormField {
                Layout.fillWidth: true
                label: "地点"
                Input { id: placeField; maximumLength: 80 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 14
            FormField {
                Layout.preferredWidth: 150
                label: "星期"
                FilterCombo {
                    id: dayCombo
                    Layout.fillWidth: true
                    model: app.schedule.dayNames
                }
            }
            FormField {
                Layout.fillWidth: true
                label: "节次"
                FilterCombo {
                    id: slotCombo
                    Layout.fillWidth: true
                    model: app.schedule.periods.map(function (p) { return p.label + "　" + p.time })
                }
            }
        }

        FormField {
            Layout.fillWidth: true
            label: "上课周次"
            error: dlg.weeksError
            hint: "例如：2　或　2-6　或　1-8单周"
            Input {
                id: weeksField
                invalid: dlg.weeksError !== ""
                onTextEdited: dlg.weeksError = ""
            }
        }

        RowLayout {
            Layout.topMargin: 6
            Layout.alignment: Qt.AlignRight
            spacing: 10
            AppButton { text: "取消"; onClicked: dlg.close() }
            AppButton { text: "新增课程"; variant: "primary"; onClicked: dlg.save() }
        }
    }
}
