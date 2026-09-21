import QtQuick
import QtQuick.Controls.Basic
import Gdipu

TextField {
    id: field
    implicitHeight: 38
    leftPadding: 38
    rightPadding: clearButton.visible ? 34 : 12
    font.family: Theme.fontUi
    font.pixelSize: Theme.textMd
    color: Theme.text
    placeholderTextColor: Theme.textFaint
    selectByMouse: true
    selectionColor: Theme.primary
    selectedTextColor: Theme.onPrimary
    verticalAlignment: TextInput.AlignVCenter

    background: Rectangle {
        radius: Theme.radiusSm + 2
        color: Theme.surface
        border.width: field.activeFocus ? 2 : 1
        border.color: field.activeFocus ? Theme.primary : Theme.border

        FluentIcon {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            glyph: Theme.icon.search
            size: 15
            color: Theme.textFaint
        }
    }

    Item {
        id: clearButton
        visible: field.text.length > 0
        width: 26
        height: 26
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        FluentIcon {
            anchors.centerIn: parent
            glyph: Theme.icon.close
            size: 12
            color: clearArea.containsMouse ? Theme.text : Theme.textFaint
        }
        MouseArea {
            id: clearArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                field.clear()
                field.forceActiveFocus()
            }
        }
    }
}
