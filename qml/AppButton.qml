import QtQuick
import QtQuick.Controls

Button {
    id: b
    property bool primary: false
    property bool danger: false
    property string iconName: ""   // optional custom SVG icon shown before the text
    readonly property var th: core.theme
    readonly property color fg: primary ? "white" : (!enabled ? th.subtext : (danger ? th.missing : th.text))

    implicitHeight: 38
    leftPadding: 14
    rightPadding: 14
    hoverEnabled: true

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 8
            Icon {
                visible: b.iconName !== ""
                anchors.verticalCenter: parent.verticalCenter
                name: b.iconName
                color: b.fg
                size: 18
            }
            Text {
                visible: text !== ""
                anchors.verticalCenter: parent.verticalCenter
                text: b.text
                font: b.font
                color: b.fg
                elide: Text.ElideRight
            }
        }
    }
    background: Rectangle {
        radius: b.th.radius === 0 ? 0 : 9
        opacity: b.enabled ? 1 : 0.6
        scale: b.down ? 0.97 : 1
        Behavior on scale { enabled: core.settings.values.animations; NumberAnimation { duration: 90 } }
        color: b.primary ? b.th.accent : (b.down ? b.th.cardHover : b.th.surface)
        border.width: b.th.style === "editorial" ? 2 : 1
        border.color: (b.hovered || b.primary) ? b.th.accent : b.th.border
    }
}
