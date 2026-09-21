import QtQuick
import QtQuick.Layouts
import Gdipu

// Transient message near the bottom of the window. Call show(text, kind).
Item {
    id: toast
    property string kind: "info"     // info | success | error
    property alias text: label.text
    readonly property color accent: kind === "success" ? Theme.success : kind === "error" ? Theme.danger : Theme.primary

    width: Math.min(parent.width - 48, 560)
    height: bubble.height
    anchors.horizontalCenter: parent.horizontalCenter
    y: parent.height
    opacity: 0
    z: 100

    function show(message, k) {
        label.text = message
        kind = k || "info"
        hideTimer.interval = kind === "error" ? 7000 : 4500
        hideTimer.restart()
        enterAnim.restart()
    }

    Timer { id: hideTimer; onTriggered: exitAnim.restart() }

    ParallelAnimation {
        id: enterAnim
        NumberAnimation { target: toast; property: "opacity"; to: 1; duration: 160 }
        NumberAnimation { target: toast; property: "y"; to: toast.parent.height - toast.height - 28; duration: 200; easing.type: Easing.OutCubic }
    }
    ParallelAnimation {
        id: exitAnim
        NumberAnimation { target: toast; property: "opacity"; to: 0; duration: 200 }
        NumberAnimation { target: toast; property: "y"; to: toast.parent.height; duration: 220; easing.type: Easing.InCubic }
    }

    Rectangle {
        id: bubble
        width: parent.width
        height: row.implicitHeight + 24
        radius: Theme.radius
        color: Theme.surface
        border.color: toast.accent
        border.width: 1

        RowLayout {
            id: row
            anchors.fill: parent
            anchors.margins: 12
            anchors.leftMargin: 16
            spacing: 12
            FluentIcon {
                glyph: toast.kind === "success" ? Theme.icon.check : toast.kind === "error" ? Theme.icon.error : Theme.icon.info
                size: 18
                color: toast.accent
                Layout.alignment: Qt.AlignTop
            }
            Text {
                id: label
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.family: Theme.fontUi
                font.pixelSize: Theme.textMd
                color: Theme.text
            }
            FluentIcon {
                glyph: Theme.icon.close
                size: 12
                color: Theme.textFaint
                Layout.alignment: Qt.AlignTop
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8
                    cursorShape: Qt.PointingHandCursor
                    onClicked: exitAnim.restart()
                }
            }
        }
    }
}
