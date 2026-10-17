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

    Overlay.modal: Rectangle { color: "#88000000" }
    background: Rectangle {
        radius: Math.min(d.th.radius, 18)
        color: d.th.surface
        border.width: Math.max(1, d.th.borderWidth)
        border.color: d.th.border
    }
    contentItem: ColumnLayout {
        spacing: 14
        Text {
            text: d.title
            Layout.fillWidth: true
            font.pixelSize: 18
            font.weight: Font.DemiBold
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
