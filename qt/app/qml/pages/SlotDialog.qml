import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

Dialog {
    id: dlg
    objectName: "slotDialog"
    property int slotDay: 1
    property int slotBlock: 0
    property int slotWeek: 1
    property string error: ""
    property var items: []
    function openFor(day, block) {
        slotDay = day
        slotBlock = block
        slotWeek = app.schedule.week
        items = app.schedule.entries(day, block)
        memoField.text = app.schedule.memo(slotWeek, day, block)
        memoTimeField.text = app.schedule.memoTime(slotWeek, day, block)
        error = ""
        open()
    }
    function saveMemo() {
        error = app.schedule.setMemoAt(slotWeek, slotDay, slotBlock, memoField.text, memoTimeField.text)
        if (error === "") close()
    }
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(560, parent.width - 32)
    height: Math.min(650, parent.height - 40)
    padding: 24
    modal: true
    closePolicy: Popup.NoAutoClose
    background: Rectangle { radius: Theme.radiusLg; color: Theme.surface; border.color: Theme.border }
    Overlay.modal: Rectangle { color: Theme.scrim }
    contentItem: ColumnLayout {
        spacing: 14
        Text {
            text: "时段提醒与备忘录"
            font.family: Theme.fontUi; font.pixelSize: Theme.textXl; font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            Layout.fillWidth: true
            text: (dlg.slotWeek > 0 ? "第 " + dlg.slotWeek + " 周 · " : "全部周次 · ")
                + app.schedule.dayNames[dlg.slotDay - 1] + " · " + app.schedule.periods[dlg.slotBlock].label
                + "\n" + app.schedule.periods[dlg.slotBlock].time
            font.family: Theme.fontUi; font.pixelSize: Theme.textSm; color: Theme.textMuted
            wrapMode: Text.Wrap
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ColumnLayout {
                width: parent.width
                spacing: 10
                Repeater {
                    model: dlg.items
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: entryColumn.implicitHeight + 24
                        radius: Theme.radiusSm
                        color: modelData.kind === "registration" ? Theme.reminderRegSoft
                             : modelData.kind === "event" ? Theme.reminderEvtSoft : Theme.surfaceAlt
                        ColumnLayout {
                            id: entryColumn
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.top: parent.top; anchors.margins: 12
                            spacing: 6
                            Text {
                                Layout.fillWidth: true
                                text: modelData.kind === "course" ? "课程" : modelData.dialogTitle
                                color: modelData.kind === "registration" ? Theme.reminderReg
                                     : modelData.kind === "event" ? Theme.reminderEvt : Theme.primary
                                font.family: Theme.fontUi; font.pixelSize: Theme.textSm; font.bold: true
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.full || modelData.title
                                textFormat: Text.PlainText
                                wrapMode: Text.Wrap
                                font.family: Theme.fontUi; font.pixelSize: Theme.textSm; color: Theme.text
                            }
                            AppButton {
                                visible: (modelData.url || "") !== ""
                                text: "查看活动详情"
                                variant: "ghost"
                                onClicked: app.openActivity(modelData.url)
                            }
                        }
                    }
                }
                Text {
                    visible: dlg.items.length === 0
                    text: "该时段暂无课程和活动提醒，可以添加备忘录。"
                    Layout.fillWidth: true; wrapMode: Text.Wrap
                    font.family: Theme.fontUi; font.pixelSize: Theme.textSm; color: Theme.textMuted
                }
                Text {
                    text: "备忘录"
                    font.family: Theme.fontUi; font.pixelSize: Theme.textMd; font.bold: true; color: Theme.memoColor
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "提醒时间"
                        font.family: Theme.fontUi; font.pixelSize: Theme.textSm; color: Theme.textMuted
                    }
                    TextField {
                        id: memoTimeField
                        objectName: "slotMemoTimeEditor"
                        Layout.preferredWidth: 130
                        enabled: dlg.slotWeek > 0
                        placeholderText: "HH:mm:ss"
                        validator: RegularExpressionValidator { regularExpression: /^(?:[01]\d|2[0-3]):[0-5]\d:[0-5]\d$/ }
                        font.family: Theme.fontUi; font.pixelSize: Theme.textMd
                        color: Theme.text
                        background: Rectangle { radius: Theme.radiusSm; color: Theme.surface; border.color: memoTimeField.activeFocus ? Theme.memoColor : Theme.border }
                    }
                    Text {
                        text: "限 " + app.schedule.periods[dlg.slotBlock].time
                        font.family: Theme.fontUi; font.pixelSize: Theme.textXs; color: Theme.textMuted
                    }
                    Item { Layout.fillWidth: true }
                }
                TextArea {
                    id: memoField
                    objectName: "slotMemoEditor"
                    Layout.fillWidth: true
                    Layout.minimumHeight: 120
                    enabled: dlg.slotWeek > 0
                    placeholderText: dlg.slotWeek > 0 ? "写下要做的事，按上方具体时间提醒…（清空并保存可删除）" : "请先选择具体周次，再添加备忘录"
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    color: Theme.text
                    font.family: Theme.fontUi; font.pixelSize: Theme.textMd
                    background: Rectangle { radius: Theme.radiusSm; color: Theme.surface; border.color: memoField.activeFocus ? Theme.memoColor : Theme.border }
                }
            }
        }
        Text {
            Layout.fillWidth: true; visible: text !== ""; text: dlg.error; color: Theme.danger
            wrapMode: Text.Wrap; font.family: Theme.fontUi; font.pixelSize: Theme.textSm
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            AppButton { text: "取消"; onClicked: dlg.close() }
            AppButton { text: "保存备忘录"; variant: "primary"; enabled: dlg.slotWeek > 0; onClicked: dlg.saveMemo() }
        }
    }
}
