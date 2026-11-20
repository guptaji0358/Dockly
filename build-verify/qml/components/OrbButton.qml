import QtQuick
import QtQuick.Controls

// Round icon button used in the HUD panels (replaces rectangular push buttons).
Item {
    id: b
    property string iconName: ""
    property string tip: ""
    property bool primary: false
    property bool danger: false
    property int diameter: 42
    signal clicked()

    readonly property var th: core.theme
    readonly property color tone: danger ? th.missing : th.accent
    width: diameter
    height: diameter
    opacity: enabled ? 1 : 0.32
    scale: m.pressed ? 0.9 : (m.containsMouse ? 1.08 : 1)
    Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 110 } }

    Rectangle {   // glow
        anchors.centerIn: parent
        width: parent.width + 10; height: width; radius: width / 2
        color: b.tone
        opacity: m.containsMouse && b.enabled ? 0.22 : 0
        visible: Fx.glow
        Behavior on opacity { enabled: Fx.animated; NumberAnimation { duration: 140 } }
    }
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: b.primary ? b.tone : (m.containsMouse ? Qt.rgba(b.tone.r, b.tone.g, b.tone.b, 0.2) : Qt.rgba(1, 1, 1, 0.06))
        border.width: 1
        border.color: b.primary || m.containsMouse ? b.tone : Qt.rgba(1, 1, 1, 0.16)
        Behavior on color { enabled: Fx.animated; ColorAnimation { duration: 120 } }
    }
    Icon {
        anchors.centerIn: parent
        name: b.iconName
        size: Math.round(b.diameter * 0.46)
        color: b.primary ? "white" : (b.danger ? b.th.missing : b.th.text)
    }
    MouseArea {
        id: m
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: b.clicked()
    }
    ToolTip.visible: m.containsMouse && b.tip !== ""
    ToolTip.delay: 350
    ToolTip.text: b.tip
}
