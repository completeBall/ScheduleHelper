import QtQuick
import QtQuick.Layouts
import Gdipu

ColumnLayout {
    id: empty
    property string icon: Theme.icon.inbox
    property string title: ""
    property string subtitle: ""
    default property alias actions: actionRow.data

    spacing: 10

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 72
        Layout.preferredHeight: 72
        radius: 36
        color: Theme.primarySoft
        FluentIcon {
            anchors.centerIn: parent
            glyph: empty.icon
            size: 30
            color: Theme.primary
        }
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 6
        text: empty.title
        font.family: Theme.fontUi
        font.pixelSize: Theme.textLg
        font.weight: Font.DemiBold
        color: Theme.text
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: 420
        text: empty.subtitle
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        font.family: Theme.fontUi
        font.pixelSize: Theme.textSm
        color: Theme.textMuted
    }
    RowLayout {
        id: actionRow
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 8
        spacing: 10
    }
}
