import QtQuick

// Window background. Tiered by the GPU setting: a gradient (low), an animated aurora shader
// (balanced) and aurora + star field + mouse parallax (high).
Item {
    id: root
    readonly property var th: core.theme
    // normalised pointer position, used for parallax
    property real px: 0.5
    property real py: 0.5
    readonly property bool soft: GraphicsInfo.api === GraphicsInfo.Software
    Binding { target: Fx; property: "softwareRenderer"; value: root.soft }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter(root.th.bg, 1.25) }
            GradientStop { position: 1.0; color: root.th.bg }
        }
    }

    ShaderEffect {
        id: aurora
        anchors.fill: parent
        visible: Fx.aurora
        property real time: 0
        property real aspect: width / Math.max(1, height)
        property color base: root.th.bg
        property color colorA: root.th.accent
        property color colorB: Qt.hsla((root.th.accent.hslHue + 0.18) % 1, 0.7, 0.5, 1)
        property real intensity: root.th.dark ? 1.0 : 0.35
        fragmentShader: "../shaders/aurora.frag.qsb"

        Timer {
            interval: Fx.auroraInterval
            repeat: true
            running: Fx.auroraLive && aurora.visible
            onTriggered: aurora.time += interval / 1000
        }
    }

    // star field (high): slow drift upward, deeper stars react more to the pointer
    Item {
        anchors.fill: parent
        visible: Fx.stars
        Repeater {
            model: Fx.stars ? 70 : 0
            delegate: Rectangle {
                id: star
                required property int index
                readonly property real depth: 0.3 + ((index * 37) % 10) / 10 * 0.9
                property real baseX: ((index * 7919) % 1000) / 1000 * root.width
                property real baseY: ((index * 104729) % 1000) / 1000 * root.height
                width: 1 + depth * 1.6
                height: width
                radius: width / 2
                color: index % 5 === 0 ? root.th.accent : "white"
                opacity: 0.18 + depth * 0.35
                x: baseX + (0.5 - root.px) * 40 * depth
                y: baseY + (0.5 - root.py) * 40 * depth
                SequentialAnimation on opacity {
                    running: Fx.live && Fx.stars
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.08; duration: 1400 + (star.index % 9) * 260; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.18 + star.depth * 0.35; duration: 1400 + (star.index % 7) * 300; easing.type: Easing.InOutSine }
                }
            }
        }
    }
}
