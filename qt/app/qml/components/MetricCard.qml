import QtQuick
import QtQuick.Layouts
import Gdipu

Card {
    id: root
    property string label: ""
    property string value: "0"
    property string icon: ""
    property color accent: Theme.primary
    property color accentSoft: Theme.primarySoft

    implicitHeight: 88
    implicitWidth: 200

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            radius: 12
            color: root.accentSoft
            FluentIcon {
                anchors.centerIn: parent
                glyph: root.icon
                size: 20
                color: root.accent
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text {
                text: root.value
                font.family: Theme.fontUi
                font.pixelSize: Theme.textMetric
                font.weight: Font.DemiBold
                color: Theme.text
            }
            Text {
                Layout.fillWidth: true
                text: root.label
                elide: Text.ElideRight
                font.family: Theme.fontUi
                font.pixelSize: Theme.textXs
                color: Theme.textMuted
            }
        }
    }
}
