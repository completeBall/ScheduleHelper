import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

Item {
    id: panel
    objectName: "todayTasksPanel"
    property bool expanded: false
    property bool compact: false
    property int expandedWidth: 390
    property int expandedHeight: 520
    property var now: new Date()
    readonly property var tasks: {
        const revision = app.schedule.revision
        const today = app.dateToday()
        const tick = panel.now
        return app.schedule.tasksForDate(today)
    }
    readonly property var groups: [
        {kind: "course", title: "课程", color: Theme.primary},
        {kind: "registration", title: "报名时间", color: Theme.reminderReg},
        {kind: "event", title: "活动时间", color: Theme.reminderEvt},
        {kind: "memo", title: "备忘录", color: Theme.memoColor}
    ]

    width: expanded ? expandedWidth : (compact ? 48 : Math.min(200, parent.width))
    height: expanded ? expandedHeight : 48
    z: 90

    function countdown(target) {
        const seconds = Math.ceil((new Date(target).getTime() - now.getTime()) / 1000)
        if (!Number.isFinite(seconds)) return "时间待确认"
        if (seconds <= 0) return "已到时间"
        const days = Math.floor(seconds / 86400)
        const hours = Math.floor(seconds % 86400 / 3600)
        const minutes = Math.floor(seconds % 3600 / 60)
        const secs = seconds % 60
        const pad = n => String(n).padStart(2, "0")
        return "倒计时 " + (days ? days + "天 " : "") + pad(hours) + ":" + pad(minutes) + ":" + pad(secs)
    }

    Timer { interval: 1000; running: true; repeat: true; onTriggered: panel.now = new Date() }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusLg
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
    }

    Item {
        anchors.fill: parent

        Rectangle {
            id: header
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 48
            radius: Theme.radiusLg
            color: panel.expanded ? Theme.primarySoft : Theme.sidebarActive
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 8
                FluentIcon {
                    visible: !panel.expanded
                    glyph: Theme.icon.bell
                    size: 18
                    color: Theme.sidebarTextActive
                }
                Text {
                    visible: panel.expanded || !panel.compact
                    text: panel.expanded ? "今日任务 · " + panel.tasks.length : "今日任务 " + panel.tasks.length
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textMd
                    font.bold: true
                    color: panel.expanded ? Theme.text : Theme.sidebarTextActive
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: panel.expanded || !panel.compact
                    text: panel.expanded ? "收起⌄" : "⌃"
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textSm
                    color: panel.expanded ? Theme.primary : Theme.sidebarTextActive
                }
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: panel.expanded = !panel.expanded
            }
        }

        ScrollView {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            visible: panel.expanded
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                x: 14
                y: 14
                width: parent.width - 28
                spacing: 12
                Repeater {
                    model: panel.groups
                    delegate: ColumnLayout {
                        required property var modelData
                        readonly property var groupTasks: panel.tasks.filter(t => t.kind === modelData.kind)
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            text: modelData.title + " · " + groupTasks.length
                            font.family: Theme.fontUi
                            font.pixelSize: modelData.kind === "course" ? Theme.textLg : Theme.textMd
                            font.bold: true
                            color: modelData.color
                        }
                        Text {
                            visible: groupTasks.length === 0
                            text: "今天暂无"
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.textSm
                            color: Theme.textFaint
                        }
                        Repeater {
                            model: groupTasks
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: taskColumn.implicitHeight + 16
                                radius: Theme.radiusSm
                                color: Theme.surfaceAlt
                                ColumnLayout {
                                    id: taskColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 8
                                    spacing: 3
                                    Text {
                                        Layout.fillWidth: true
                                        text: (modelData.kind === "memo" ? "🔴 " : "") + modelData.title
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        font.family: Theme.fontUi
                                        font.pixelSize: Theme.textSm
                                        font.bold: modelData.kind === "course"
                                        color: modelData.kind === "memo" ? Theme.reminderEvt : Theme.text
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: modelData.time
                                            font.family: Theme.fontUi
                                            font.pixelSize: Theme.textXs
                                            color: Theme.textMuted
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: panel.countdown(modelData.target)
                                            font.family: Theme.fontUi
                                            font.pixelSize: Theme.textXs
                                            color: modelData.kind === "memo" ? Theme.reminderEvt : Theme.primary
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
