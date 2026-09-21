import QtQuick
import QtQuick.Controls.Basic
import Gdipu

Button {
    id: mark
    property color tint: Theme.reminderReg
    implicitWidth: 26
    implicitHeight: 36
    padding: 0
    Accessible.name: text
    ToolTip.visible: hovered
    ToolTip.text: text
    background: Canvas {
        id: shape
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = mark.down ? Qt.darker(mark.tint, 1.15) : mark.tint
            ctx.beginPath()
            ctx.moveTo(3, 1); ctx.lineTo(width - 3, 1)
            ctx.lineTo(width - 3, height - 2); ctx.lineTo(width / 2, height - 9)
            ctx.lineTo(3, height - 2); ctx.closePath(); ctx.fill()
        }
        Connections {
            target: mark
            function onTintChanged() { shape.requestPaint() }
            function onDownChanged() { shape.requestPaint() }
        }
    }
    contentItem: Text {
        text: mark.activeFocus ? "•" : ""
        color: "white"
        horizontalAlignment: Text.AlignHCenter
    }
}
