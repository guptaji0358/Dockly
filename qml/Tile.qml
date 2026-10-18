import QtQuick
import QtQuick.Shapes

// One item as a floating glass hexagon. It drifts, tilts toward the pointer (the icon moves the
// opposite way for depth) and glows in its category colour. Effects follow the GPU level.
Item {
    id: root
    required property string itemId
    required property string name
    required property string subtitle
    required property string target
    required property string iconSource
    required property int status
    required property int category
    required property bool favorite
    required property int index

    property real radius: 78            // hexagon circumradius
    property bool selected: false
    signal opened()
    signal clicked()
    signal menuRequested()
    signal favoriteRequested()

    readonly property var th: core.theme
    readonly property color accent: th.accentOf(category)
    readonly property real hexW: Math.sqrt(3) * radius
    readonly property real hexH: 2 * radius
    readonly property bool hovered: mouse.containsMouse

    width: hexW
    height: hexH
    z: hovered ? 5 : (selected ? 3 : 0)

    // 0..1 hover progress
    property real t: hovered ? 1 : 0
    Behavior on t { enabled: Fx.animated; NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
    readonly property real lit: Math.max(t, selected ? 1 : 0)

    // pointer position inside the tile, -1..1
    readonly property real mx: hovered ? (mouse.mouseX / hexW - 0.5) * 2 : 0
    readonly property real my: hovered ? (mouse.mouseY / hexH - 0.5) * 2 : 0

    // idle floating, desynchronised per tile
    property real bob: 0
    SequentialAnimation on bob {
        running: Fx.floating && Fx.live && root.visible
        loops: Animation.Infinite
        NumberAnimation { from: -3.5; to: 3.5; duration: 1700 + (root.index % 7) * 130; easing.type: Easing.InOutSine }
        NumberAnimation { from: 3.5; to: -3.5; duration: 1700 + (root.index % 7) * 130; easing.type: Easing.InOutSine }
    }

    // entrance
    property real appear: Fx.animated ? 0 : 1
    Component.onCompleted: if (Fx.animated) enter.start()
    SequentialAnimation {
        id: enter
        PauseAnimation { duration: Math.min(root.index, 30) * 22 }
        NumberAnimation { target: root; property: "appear"; to: 1; duration: 420; easing.type: Easing.OutBack }
    }

    opacity: Math.min(1, root.appear * 1.6)
    scale: (0.55 + 0.45 * appear) * (1 + 0.07 * t)
    transform: [
        Translate { y: (Fx.floating ? root.bob * (1 - root.t) : 0) - 5 * root.t },
        Rotation {
            origin.x: root.hexW / 2; origin.y: root.hexH / 2
            axis { x: 0; y: 1; z: 0 }
            angle: Fx.tilt ? root.mx * 16 * root.t : 0
        },
        Rotation {
            origin.x: root.hexW / 2; origin.y: root.hexH / 2
            axis { x: 1; y: 0; z: 0 }
            angle: Fx.tilt ? -root.my * 16 * root.t : 0
        }
    ]

    // soft glow under the tile
    Rectangle {
        visible: Fx.glow
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 6 + 8 * root.lit
        width: root.hexW * 0.7
        height: root.hexH * 0.55
        radius: height / 2
        color: root.accent
        opacity: (Fx.haloGlow ? 0.10 : 0.05) + 0.22 * root.lit
        scale: 1.25
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        // outer halo
        ShapePath {
            strokeWidth: 6
            strokeColor: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.20 * root.lit + (Fx.haloGlow ? 0.06 : 0))
            fillColor: "transparent"
            joinStyle: ShapePath.RoundJoin
            startX: root.hexW / 2; startY: -1
            PathLine { x: root.hexW + 1; y: root.radius / 2 - 1 }
            PathLine { x: root.hexW + 1; y: root.radius * 1.5 + 1 }
            PathLine { x: root.hexW / 2; y: root.hexH + 1 }
            PathLine { x: -1; y: root.radius * 1.5 + 1 }
            PathLine { x: -1; y: root.radius / 2 - 1 }
            PathLine { x: root.hexW / 2; y: -1 }
        }
        // glass body
        ShapePath {
            strokeWidth: 1.5 + root.lit
            strokeColor: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.45 + 0.55 * root.lit)
            joinStyle: ShapePath.RoundJoin
            fillGradient: LinearGradient {
                y1: 0; y2: root.hexH
                GradientStop { position: 0.0; color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.20 + 0.18 * root.lit) }
                GradientStop { position: 1.0; color: Qt.rgba(root.th.surface.r, root.th.surface.g, root.th.surface.b, 0.78) }
            }
            startX: root.hexW / 2; startY: 1
            PathLine { x: root.hexW - 1; y: root.radius / 2 }
            PathLine { x: root.hexW - 1; y: root.radius * 1.5 }
            PathLine { x: root.hexW / 2; y: root.hexH - 1 }
            PathLine { x: 1; y: root.radius * 1.5 }
            PathLine { x: 1; y: root.radius / 2 }
            PathLine { x: root.hexW / 2; y: 1 }
        }
    }

    // icon (moves against the tilt for parallax depth)
    Item {
        id: iconBox
        readonly property real sz: root.radius * 0.78
        width: sz
        height: sz
        x: (root.hexW - sz) / 2 - (Fx.tilt ? root.mx * 7 * root.t : 0)
        y: root.hexH * 0.2 - 3 * root.t - (Fx.tilt ? root.my * 6 * root.t : 0)
        Image {
            anchors.fill: parent
            source: root.iconSource
            sourceSize: Qt.size(iconBox.sz * 2, iconBox.sz * 2)
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            opacity: root.status === 0 ? 1 : 0.4
        }
        Icon {
            anchors.centerIn: parent
            visible: root.status !== 0
            name: "warning"
            size: Math.round(iconBox.sz * 0.6)
            color: root.status === 2 ? root.th.missing : root.th.warning
        }
    }

    Text {
        text: root.name
        color: root.th.text
        font.pixelSize: 12
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignTop
        wrapMode: Text.Wrap
        maximumLineCount: 2
        elide: Text.ElideRight
        width: root.hexW * 0.74
        x: (root.hexW - width) / 2
        y: iconBox.y + iconBox.sz + 8
    }

    // favourite mark / toggle
    Item {
        visible: root.favorite || root.hovered
        z: 10
        width: 22
        height: 22
        x: root.hexW / 2 - width / 2
        y: 3
        opacity: root.favorite ? 1 : 0.7
        Icon {
            anchors.centerIn: parent
            name: root.favorite ? "star-fill" : "star"
            size: 15
            color: root.favorite ? root.th.favorite : root.th.text
        }
        MouseArea {
            anchors.fill: parent
            anchors.margins: -4
            cursorShape: Qt.PointingHandCursor
            onClicked: root.favoriteRequested()
        }
    }

    // pointer area follows the hexagon, not its bounding box (neighbouring tiles overlap)
    Item {
        id: hexMask
        function contains(p) {
            const w = root.hexW, r = root.radius
            const dx = Math.abs(p.x - w / 2), dy = Math.abs(p.y - r)
            return dx <= w / 2 && dy <= r - dx * r / w
        }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        containmentMask: hexMask
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: m => { if (m.button === Qt.RightButton) root.menuRequested(); else root.clicked() }
        onDoubleClicked: m => { if (m.button === Qt.LeftButton) root.opened() }
    }
}
