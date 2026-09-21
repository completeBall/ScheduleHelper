import QtQuick
import Gdipu

// A bordered surface. Children go in via the default `content` item.
Rectangle {
    id: card
    default property alias content: inner.data
    property int padding: 0
    property bool hoverable: false
    property alias hovered: hover.hovered

    color: hoverable && hover.hovered ? Theme.surfaceHover : Theme.surface
    radius: Theme.radius
    border.color: Theme.border
    border.width: 1
    Behavior on color { ColorAnimation { duration: 120 } }

    HoverHandler { id: hover }

    Item {
        id: inner
        anchors.fill: parent
        anchors.margins: card.padding
    }
}
