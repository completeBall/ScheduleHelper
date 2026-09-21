import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

// Sidebar entry. Collapses to icon-only when `compact`.
AbstractButton {
    id: nav
    property string glyph: ""
    property bool compact: false
    property bool active: false
    property bool busy: false

    implicitHeight: 44
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    HoverHandler { cursorShape: Qt.PointingHandCursor }

    ToolTip.visible: compact && hovered
    ToolTip.text: text
    ToolTip.delay: 400

    background: Rectangle {
        radius: Theme.radius
        color: nav.active ? Theme.sidebarActive : nav.hovered ? Theme.sidebarHover : "transparent"
        Behavior on color { ColorAnimation { duration: 100 } }

        Rectangle {
            visible: nav.active
            width: 3
            height: 18
            radius: 2
            color: "#5fe0d3"
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    leftPadding: compact ? 0 : 16
    rightPadding: 0

    contentItem: RowLayout {
        spacing: 12
        Item {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: nav.compact
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
            FluentIcon {
                anchors.centerIn: parent
                glyph: nav.glyph
                size: 18
                color: nav.active ? Theme.sidebarTextActive : Theme.sidebarText
            }
        }
        Text {
            visible: !nav.compact
            Layout.fillWidth: true
            text: nav.text
            font.family: Theme.fontUi
            font.pixelSize: Theme.textMd
            font.weight: nav.active ? Font.DemiBold : Font.Normal
            color: nav.active ? Theme.sidebarTextActive : Theme.sidebarText
            elide: Text.ElideRight
        }
        Rectangle {
            visible: nav.busy && !nav.compact
            Layout.preferredWidth: 8
            Layout.preferredHeight: 8
            Layout.rightMargin: 14
            radius: 4
            color: "#5fe0d3"
            SequentialAnimation on opacity {
                running: nav.busy
                loops: Animation.Infinite
                NumberAnimation { from: 1; to: 0.25; duration: 700 }
                NumberAnimation { from: 0.25; to: 1; duration: 700 }
            }
        }
    }
}
