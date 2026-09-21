import QtQuick
import QtQuick.Controls.Basic
import Gdipu

ComboBox {
    id: combo
    implicitHeight: 38
    implicitWidth: 168
    hoverEnabled: true
    font.family: Theme.fontUi
    font.pixelSize: Theme.textMd
    leftPadding: 12
    rightPadding: 34

    HoverHandler { cursorShape: Qt.PointingHandCursor }

    contentItem: Text {
        text: combo.displayText
        font: combo.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: FluentIcon {
        x: combo.width - width - 12
        y: (combo.height - height) / 2
        glyph: Theme.icon.chevronDown
        size: 12
        color: Theme.textMuted
    }

    background: Rectangle {
        radius: Theme.radiusSm + 2
        color: combo.hovered || combo.popup.visible ? Theme.surfaceHover : Theme.surface
        border.width: combo.visualFocus || combo.popup.visible ? 2 : 1
        border.color: combo.visualFocus || combo.popup.visible ? Theme.primary : Theme.border
    }

    delegate: ItemDelegate {
        id: item
        required property var modelData
        required property int index
        width: combo.width
        height: 36
        highlighted: combo.highlightedIndex === index
        contentItem: Text {
            text: item.modelData
            font: combo.font
            color: combo.currentIndex === item.index ? Theme.primary : Theme.text
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: Theme.radiusSm
            color: item.highlighted ? Theme.surfaceAlt : "transparent"
        }
    }

    popup: Popup {
        y: combo.height + 4
        width: Math.max(combo.width, 180)
        implicitHeight: Math.min(contentItem.implicitHeight + 12, 320)
        padding: 6

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: combo.popup.visible ? combo.delegateModel : null
            currentIndex: combo.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }
    }
}
