import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

// Status line for long-running work. Shows a progress bar while `running`.
Rectangle {
    id: banner
    property string message: ""
    property bool running: false
    property real progress: -1           // 0..1, or -1 for indeterminate
    property string kind: "info"         // info | error
    signal stopClicked

    visible: message !== ""
    implicitHeight: visible ? col.implicitHeight + 24 : 0
    radius: Theme.radius
    color: kind === "error" ? Theme.dangerSoft : Theme.primarySoft
    border.color: "transparent"

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: 12
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            FluentIcon {
                glyph: banner.running ? Theme.icon.refresh : banner.kind === "error" ? Theme.icon.warning : Theme.icon.info
                size: 15
                color: banner.kind === "error" ? Theme.danger : Theme.primary
                RotationAnimator on rotation {
                    running: banner.running
                    from: 0
                    to: 360
                    duration: 1200
                    loops: Animation.Infinite
                }
            }
            Text {
                Layout.fillWidth: true
                text: banner.message
                elide: Text.ElideRight
                font.family: Theme.fontUi
                font.pixelSize: Theme.textSm
                color: banner.kind === "error" ? Theme.danger : Theme.text
            }
        }

        ProgressBar {
            id: bar
            visible: banner.running
            Layout.fillWidth: true
            Layout.preferredHeight: 4
            indeterminate: banner.progress < 0
            value: Math.max(0, banner.progress)

            background: Rectangle {
                radius: 2
                color: Theme.surface
            }
            // The sweep starts and ends outside the track, so the track must clip it.
            contentItem: Item {
                id: track
                clip: true

                Rectangle {
                    id: fill
                    visible: !bar.indeterminate
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: bar.value * parent.width
                    radius: 2
                    color: Theme.primary
                }
                Rectangle {
                    id: sweep
                    visible: bar.indeterminate
                    width: track.width * 0.3
                    height: track.height
                    radius: 2
                    color: Theme.primary
                    SequentialAnimation on x {
                        running: bar.indeterminate && banner.running
                        loops: Animation.Infinite
                        NumberAnimation { from: -sweep.width; to: track.width; duration: 1100; easing.type: Easing.InOutQuad }
                    }
                }
            }
        }
    }
}
