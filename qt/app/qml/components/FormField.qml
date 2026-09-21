import QtQuick
import QtQuick.Layouts
import Gdipu

// Label above an input, with an optional inline error underneath.
ColumnLayout {
    id: field
    property string label: ""
    property string error: ""
    property string hint: ""
    default property alias content: slot.data

    spacing: 6

    Text {
        text: field.label
        font.family: Theme.fontUi
        font.pixelSize: Theme.textSm
        font.weight: Font.Medium
        color: Theme.textMuted
    }

    ColumnLayout {
        id: slot
        Layout.fillWidth: true
        spacing: 0
    }

    Text {
        visible: field.error !== "" || field.hint !== ""
        Layout.fillWidth: true
        text: field.error !== "" ? field.error : field.hint
        wrapMode: Text.Wrap
        font.family: Theme.fontUi
        font.pixelSize: Theme.textXs
        color: field.error !== "" ? Theme.danger : Theme.textFaint
    }
}
