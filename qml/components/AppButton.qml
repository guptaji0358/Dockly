import QtQuick
import QtQuick.Controls

Button {
    id: b
    property bool primary: false
    property bool danger: false
    property string iconName: ""   // optional custom SVG icon shown before the text
    readonly property var th: core.theme
    readonly property color fgTone: danger ? th.missing : th.accent
    readonly property color fg: primary ? "white" : (!enabled ? th.subtext : (danger ? th.missing : th.text))

    implicitHeight: 38
    leftPadding: 18
    rightPadding: 18
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
        radius: height / 2
        opacity: b.enabled ? 1 : 0.45
        scale: b.down ? 0.97 : 1
        Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 90 } }
        gradient: b.primary ? primaryGradient : null
        color: b.primary ? "transparent"
               : (b.hovered ? Qt.rgba(b.fgTone.r, b.fgTone.g, b.fgTone.b, 0.16) : Qt.rgba(1, 1, 1, 0.06))
        border.width: 1
        border.color: (b.hovered || b.primary) ? b.fgTone : Qt.rgba(1, 1, 1, 0.16)
        Gradient {
            id: primaryGradient
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Qt.lighter(b.th.accent, 1.2) }
            GradientStop { position: 1; color: Qt.darker(b.th.accent, 1.2) }
        }
    }
}
