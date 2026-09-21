import QtQuick
import Gdipu

// kind: open | soon | closed | error | neutral
Rectangle {
    id: badge
    property string kind: "neutral"
    property alias text: label.text

    readonly property color fg: kind === "open" ? Theme.success
                              : kind === "soon" ? Theme.info
                              : kind === "error" ? Theme.danger
                              : Theme.neutralFg
    readonly property color soft: kind === "open" ? Theme.successSoft
                                : kind === "soon" ? Theme.infoSoft
                                : kind === "error" ? Theme.dangerSoft
                                : Theme.neutralSoft

    implicitWidth: label.implicitWidth + 20
    implicitHeight: 26
    radius: height / 2
    color: soft

    Row {
        anchors.centerIn: parent
        spacing: 6
        Rectangle {
            width: 6
            height: 6
            radius: 3
            color: badge.fg
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: label
            font.family: Theme.fontUi
            font.pixelSize: Theme.textXs
            font.weight: Font.Medium
            color: badge.fg
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
