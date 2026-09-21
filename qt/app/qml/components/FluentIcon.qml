import QtQuick
import Gdipu

// One glyph from the Windows icon font (see Theme.icon).
Text {
    property string glyph: ""
    property int size: 16

    text: glyph
    font.family: Theme.fontIcon
    font.pixelSize: size
    color: Theme.text
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    renderType: Text.NativeRendering
}
