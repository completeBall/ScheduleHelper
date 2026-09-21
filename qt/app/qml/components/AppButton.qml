import QtQuick
import QtQuick.Controls.Basic
import Gdipu

Button {
    id: control
    property string variant: "secondary"      // primary | secondary | ghost | danger
    property string glyph: ""
    property int iconSize: 16

    readonly property color fg: {
        if (variant === "primary") return Theme.onPrimary
        if (variant === "danger") return Theme.danger
        if (variant === "ghost") return Theme.textMuted
        return Theme.text
    }

    implicitHeight: 38
    leftPadding: 16
    rightPadding: 16
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    font.family: Theme.fontUi
    font.pixelSize: Theme.textMd
    opacity: enabled ? 1 : 0.45

    HoverHandler { cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 8
            FluentIcon {
                visible: control.glyph !== ""
                glyph: control.glyph
                size: control.iconSize
                color: control.fg
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                visible: control.text !== ""
                text: control.text
                font: control.font
                color: control.fg
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusSm + 2
        color: {
            const down = control.down, over = control.hovered
            if (control.variant === "primary")
                return down ? Theme.primaryPressed : over ? Theme.primaryHover : Theme.primary
            if (control.variant === "danger")
                return down || over ? Theme.dangerSoft : "transparent"
            if (control.variant === "ghost")
                return down ? Theme.surfaceAlt : over ? Theme.surfaceHover : "transparent"
            return down ? Theme.surfaceAlt : over ? Theme.surfaceHover : Theme.surface
        }
        border.width: control.variant === "secondary" || control.variant === "danger" ? 1 : 0
        border.color: control.variant === "danger" ? Theme.danger : (control.visualFocus ? Theme.primary : Theme.border)
        Behavior on color { ColorAnimation { duration: 100 } }
    }
}
