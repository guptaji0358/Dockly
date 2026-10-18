import QtQuick

// Frosted-glass panel: translucent fill, a light sheen along the top edge and a hairline border.
// Cheap on purpose (no live blur), so it costs nothing on any GPU.
Rectangle {
    id: glass
    property real tint: 0.62          // fill opacity
    property color edge: Qt.rgba(1, 1, 1, 0.14)
    property bool brackets: false     // sci-fi corner marks
    readonly property var th: core.theme

    radius: 16
    color: Qt.rgba(th.surface.r, th.surface.g, th.surface.b, tint)
    border.width: 1
    border.color: edge

    Rectangle {   // top sheen
        anchors.fill: parent
        anchors.margins: 1
        radius: glass.radius - 1
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, glass.th.dark ? 0.07 : 0.35) }
            GradientStop { position: 0.45; color: "transparent" }
        }
    }

    Repeater {
        model: glass.brackets ? 4 : 0
        delegate: Item {
            required property int index
            width: 12
            height: 12
            x: index % 2 === 0 ? -1 : glass.width - width + 1
            y: index < 2 ? -1 : glass.height - height + 1
            Rectangle { width: parent.width; height: 2; color: glass.th.accent; y: index < 2 ? 0 : parent.height - 2 }
            Rectangle { height: parent.height; width: 2; color: glass.th.accent; x: index % 2 === 0 ? 0 : parent.width - 2 }
        }
    }
}
