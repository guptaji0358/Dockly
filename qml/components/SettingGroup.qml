import QtQuick
import QtQuick.Layouts

// A titled card that holds a column of settings.
Rectangle {
    id: g
    property string title
    default property alias rows: col.data

    Layout.fillWidth: true
    implicitHeight: col.implicitHeight + 32
    radius: 12
    color: core.theme.surface
    border.color: core.theme.border

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: 18
        anchors.topMargin: 14
        anchors.bottomMargin: 16
        spacing: 10
        Text {
            text: g.title
            color: core.theme.text
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
    }
}
