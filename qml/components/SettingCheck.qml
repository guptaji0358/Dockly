import QtQuick
import QtQuick.Controls

CheckBox {
    id: box
    property string key
    readonly property var th: core.theme

    checked: core.settings.values[key] === true
    onToggled: core.settings.set(key, checked)
    Connections {
        target: core.settings
        function onValuesChanged() { box.checked = core.settings.values[box.key] === true }
    }

    spacing: 10
    contentItem: Text {
        text: box.text
        color: box.th.text
        verticalAlignment: Text.AlignVCenter
        leftPadding: box.indicator.width + box.spacing
    }
    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: 0
        y: (box.height - height) / 2
        radius: 5
        color: box.checked ? box.th.accent : box.th.surface
        border.width: 1.5
        border.color: box.checked || box.hovered ? box.th.accent : box.th.subtext
        Text {
            anchors.centerIn: parent
            visible: box.checked
            text: "✓"
            color: "white"
            font.pixelSize: 13
            font.bold: true
        }
    }
}
