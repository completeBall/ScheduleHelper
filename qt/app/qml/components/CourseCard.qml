import QtQuick
import QtQuick.Layouts
import Gdipu

// One card inside a timetable cell: a course, a manual course, or an activity reminder.
Rectangle {
    id: card
    property var info: ({})
    signal clicked
    signal removeRequested

    readonly property bool isReminder: info.kind === "registration" || info.kind === "event"
    readonly property var palette: Theme.course(info.color || 0)
    readonly property color bgColor: info.kind === "registration" ? Theme.reminderRegSoft
                                   : info.kind === "event" ? Theme.reminderEvtSoft
                                   : palette[0]
    readonly property color accent: info.kind === "registration" ? Theme.reminderReg
                                  : info.kind === "event" ? Theme.reminderEvt
                                  : palette[1]

    width: parent ? parent.width : 100
    height: col.implicitHeight + 14
    radius: Theme.radiusSm + 1
    color: hover.hovered ? Qt.lighter(bgColor, Theme.dark ? 1.15 : 1.02) : bgColor
    border.color: hover.hovered ? accent : "transparent"
    border.width: 1

    Rectangle {
        width: 3
        height: parent.height - 14
        radius: 2
        color: card.accent
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
    }

    HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: card.clicked() }

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 14
        anchors.rightMargin: 8
        anchors.topMargin: 7
        spacing: 3

        // reminder: "🔔 报名开始 10:30"
        RowLayout {
            visible: card.isReminder
            Layout.fillWidth: true
            spacing: 5
            FluentIcon {
                glyph: card.info.kind === "registration" ? Theme.icon.bell : Theme.icon.calendar
                size: 12
                color: card.accent
            }
            Text {
                Layout.fillWidth: true
                text: card.info.label || ""
                elide: Text.ElideRight
                font.family: Theme.fontUi
                font.pixelSize: Theme.textXs
                font.weight: Font.DemiBold
                color: card.accent
            }
        }

        // course name / activity name
        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            Text {
                Layout.fillWidth: true
                text: card.info.title || ""
                wrapMode: Text.Wrap
                maximumLineCount: card.isReminder ? 2 : 2
                elide: Text.ElideRight
                font.family: Theme.fontUi
                font.pixelSize: Theme.textSm
                font.weight: card.isReminder ? Font.Medium : Font.DemiBold
                color: Theme.text
            }
            FluentIcon {
                visible: card.info.manual === true
                glyph: Theme.icon.remove
                size: 12
                color: hoverRemove.hovered ? Theme.danger : Theme.textFaint
                HoverHandler { id: hoverRemove; cursorShape: Qt.PointingHandCursor }
                TapHandler {
                    gesturePolicy: TapHandler.ReleaseWithinBounds
                    onTapped: card.removeRequested()
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: text !== ""
            text: card.info.detail || ""
            wrapMode: Text.Wrap
            maximumLineCount: 4
            elide: Text.ElideRight
            lineHeight: 1.15
            font.family: Theme.fontUi
            font.pixelSize: Theme.textXs
            color: Theme.textMuted
        }

        Text {
            visible: card.info.manual === true || card.info.weeksUnknown === true
            text: card.info.manual === true ? "手动添加" : "周次未识别"
            font.family: Theme.fontUi
            font.pixelSize: 11
            color: Theme.reminderReg
        }
    }
}
