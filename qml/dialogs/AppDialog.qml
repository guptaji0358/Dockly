import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Modal card-style dialog. Children go into the body column below the title.
Popup {
    id: d
    property string title
    default property alias body: bodyCol.data
    readonly property var th: core.theme

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(520, (Overlay.overlay ? Overlay.overlay.width : 520) - 40)
    padding: 22
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle { color: Qt.rgba(d.th.bg.r, d.th.bg.g, d.th.bg.b, 0.72) }
    enter: Transition {
        enabled: Fx.animated
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
            NumberAnimation { property: "scale"; from: 0.93; to: 1; duration: 240; easing.type: Easing.OutBack }
        }
    }
    background: Glass {
        radius: 22
        tint: 0.96
        brackets: true
        edge: Qt.rgba(d.th.accent.r, d.th.accent.g, d.th.accent.b, 0.4)
    }
    contentItem: ColumnLayout {
        spacing: 14
        Text {
            visible: d.title !== ""
            text: d.title.toUpperCase()
            Layout.fillWidth: true
            font.pixelSize: 15
            font.weight: Font.Bold
            font.letterSpacing: 3
            color: d.th.text
            wrapMode: Text.Wrap
        }
        ColumnLayout {
            id: bodyCol
            Layout.fillWidth: true
            spacing: 10
        }
    }
}
