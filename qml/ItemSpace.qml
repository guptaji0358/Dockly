import QtQuick
import QtQuick.Controls

// Items as a spatial map: a honeycomb or concentric orbits of glass tiles on a pan / zoom canvas.
// Drag the background to pan, wheel to zoom, double-click the background (or press Fit) to re-centre.
// In "grid" layout (or when styleOverride is set) it falls back to the classic card grid / list.
Item {
    id: root
    property var model
    property string emptyText: ""
    property string styleOverride: ""
    property string selectedId: ""
    property string collectionId: ""
    property string coreLabel: ""     // shown in the middle of the orbit layout
    property bool strip: false        // one horizontal, pannable row (used for "Recent")
    property var host
    signal itemClicked(string itemId)

    readonly property string mode: strip ? "strip" : (styleOverride !== "" ? "grid" : core.settings.values.layoutMode)
    readonly property bool spatial: mode === "hex" || mode === "orbit" || mode === "strip"
    readonly property string cardSize: core.settings.values.cardSize
    readonly property real tr: strip ? 46 : (cardSize === "small" ? 56 : (cardSize === "large" ? 90 : 72))
    readonly property real tw: Math.sqrt(3) * tr
    readonly property int count: rep.count
    property var pts: []
    property real spin: 0
    property bool userMoved: false
    property bool fitting: false

    // ---- layout -------------------------------------------------------------------

    function build() {
        const n = rep.count
        const out = []
        if (mode === "strip") {
            for (let i = 0; i < n; ++i)
                out.push({x: i * (tw + 8), y: 0, R: 0, a: 0, dir: 0})
        } else if (mode === "hex") {
            const sz = tr + 6
            const dirs = [[1, 0], [1, -1], [0, -1], [-1, 0], [-1, 1], [0, 1]]
            const cells = [[0, 0]]
            for (let k = 1; cells.length < n; ++k) {
                let q = -k, r = k
                for (let side = 0; side < 6; ++side)
                    for (let s = 0; s < k; ++s) {
                        cells.push([q, r])
                        q += dirs[side][0]
                        r += dirs[side][1]
                    }
            }
            for (let i = 0; i < n; ++i) {
                const q = cells[i][0], r = cells[i][1]
                out.push({x: sz * Math.sqrt(3) * (q + r / 2), y: sz * 1.5 * r, R: 0, a: 0, dir: 0})
            }
        } else {
            const pitch = tw * 1.12
            let ring = 0, placed = 0
            while (placed < n) {
                const R = tr * 2.5 + ring * (tr * 2.15)
                const cap = Math.max(3, Math.floor(2 * Math.PI * R / pitch))
                const take = Math.min(cap, n - placed)
                const dir = ring % 2 === 0 ? 1 : -1
                const a0 = -Math.PI / 2 + ring * 0.5
                for (let j = 0; j < take; ++j) {
                    const a = a0 + 2 * Math.PI * j / take
                    out.push({x: R * Math.cos(a), y: R * Math.sin(a), R: R, a: a, dir: dir / (1 + ring * 0.35)})
                }
                placed += take
                ++ring
            }
        }
        pts = out
        Qt.callLater(fit)
    }

    function panBy(dx, dy) {
        fitting = false
        userMoved = true
        world.x += dx
        world.y += dy
    }

    function fit() {
        if (!spatial || width < 10 || height < 10)
            return
        if (mode === "strip") {
            fitting = false
            world.scale = 1
            world.x = tw / 2 + 6
            world.y = height / 2
            return
        }
        let x0 = -tr, x1 = tr, y0 = -tr, y1 = tr
        const orbit = mode === "orbit"
        for (const p of pts) {
            const reach = orbit ? 0 : 0
            x0 = Math.min(x0, p.x - tw / 2 - reach); x1 = Math.max(x1, p.x + tw / 2 + reach)
            y0 = Math.min(y0, p.y - tr); y1 = Math.max(y1, p.y + tr)
        }
        const bw = x1 - x0 + 60, bh = y1 - y0 + 60
        const s = Math.max(0.3, Math.min(width / bw, height / bh, 1.1))
        fitting = Fx.animated
        world.scale = s
        world.x = width / 2 - (x0 + x1) / 2 * s
        world.y = height / 2 - (y0 + y1) / 2 * s
        userMoved = false
        fitDone.restart()
    }
    Timer { id: fitDone; interval: 450; onTriggered: root.fitting = false }

    onModeChanged: build()
    onTrChanged: build()
    onModelChanged: Qt.callLater(build)
    // Re-centre after the window / panel changes size, but not instantly: opening the inspector on the
    // first click of a double-click must not move the tile away from under the second click.
    onWidthChanged: if (!userMoved) refit.restart()
    onHeightChanged: if (!userMoved) refit.restart()
    Timer { id: refit; interval: 700; onTriggered: if (!root.userMoved) root.fit() }
    Component.onCompleted: Qt.callLater(build)

    NumberAnimation on spin {
        running: Fx.spin && Fx.live && root.mode === "orbit" && root.visible
        from: 0; to: 360; duration: 420000; loops: Animation.Infinite
    }

    // ---- spatial canvas ---------------------------------------------------------------

    Item {
        id: canvas
        anchors.fill: parent
        visible: root.spatial
        clip: true

        // empty background: double-click re-centres. A plain MouseArea sitting under the tiles (a TapHandler
        // on the canvas would steal clicks from the tiles).
        MouseArea {
            id: bg
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton
            property point lastG
            onPressed: m => lastG = mapToGlobal(m.x, m.y)
            onPositionChanged: m => {
                if (!pressed)
                    return
                const g = mapToGlobal(m.x, m.y)
                root.panBy(g.x - lastG.x, g.y - lastG.y)
                lastG = g
            }
            onDoubleClicked: root.fit()
        }

        Item {
            id: world
            transformOrigin: Item.TopLeft
            Behavior on x { enabled: root.fitting; NumberAnimation { duration: 380; easing.type: Easing.OutCubic } }
            Behavior on y { enabled: root.fitting; NumberAnimation { duration: 380; easing.type: Easing.OutCubic } }
            Behavior on scale { enabled: root.fitting; NumberAnimation { duration: 380; easing.type: Easing.OutCubic } }

            // glowing core + guide rings of the orbit layout
            Item {
                visible: root.mode === "orbit"
                Repeater {
                    model: root.mode === "orbit" ? 6 : 0
                    delegate: Rectangle {
                        required property int index
                        readonly property real r: root.tr * 2.5 + index * (root.tr * 2.15)
                        x: -r; y: -r; width: r * 2; height: r * 2
                        radius: r
                        color: "transparent"
                        border.width: 1
                        border.color: Qt.rgba(core.theme.accent.r, core.theme.accent.g, core.theme.accent.b, 0.10)
                        visible: root.pts.length > 0 && r < (root.pts.reduce((m, p) => Math.max(m, p.R), 0) + 1)
                    }
                }
                Rectangle {
                    x: -root.tr * 1.5; y: -root.tr * 1.5; width: root.tr * 3; height: root.tr * 3
                    radius: width / 2
                    color: Qt.rgba(core.theme.accent.r, core.theme.accent.g, core.theme.accent.b, 0.07)
                    border.width: 2
                    border.color: Qt.rgba(core.theme.accent.r, core.theme.accent.g, core.theme.accent.b, 0.35)
                    Column {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.count
                            color: core.theme.accent
                            font.pixelSize: root.tr * 0.7
                            font.weight: Font.Light
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.coreLabel
                            color: core.theme.subtext
                            font.pixelSize: 11
                            font.letterSpacing: 2
                        }
                    }
                }
            }

            Repeater {
                id: rep
                model: root.spatial ? root.model : null
                onCountChanged: Qt.callLater(root.build)

                delegate: Tile {
                    id: tile
                    readonly property var p: root.pts[index] || ({x: 0, y: 0, R: 0, a: 0, dir: 0})
                    readonly property bool turning: root.mode === "orbit" && Fx.spin
                    radius: root.tr
                    x: (turning ? p.R * Math.cos(p.a + p.dir * root.spin * Math.PI / 180) : p.x) - hexW / 2
                    y: (turning ? p.R * Math.sin(p.a + p.dir * root.spin * Math.PI / 180) : p.y) - hexH / 2
                    selected: root.selectedId === itemId || core.selectedId === itemId
                    onOpened: root.host.launch(itemId)
                    onMenuRequested: root.host.showMenu(itemId, root.collectionId)
                    onClicked: {
                        core.select(itemId)
                        root.itemClicked(itemId)
                    }
                    onFavoriteRequested: core.setFavorite(itemId, !favorite)
                    onPanned: (dx, dy) => root.panBy(dx, dy)
                }
            }
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: event => {
                root.fitting = false
                root.userMoved = true
                const f = event.angleDelta.y > 0 ? 1.12 : 1 / 1.12
                const ns = Math.max(0.25, Math.min(2.6, world.scale * f))
                const px = point.position.x, py = point.position.y
                const wx = (px - world.x) / world.scale, wy = (py - world.y) / world.scale
                world.scale = ns
                world.x = px - wx * ns
                world.y = py - wy * ns
            }
        }

        // zoom controls
        Row {
            visible: !root.strip
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 6
            spacing: 6
            Repeater {
                model: [{t: "−", k: "out"}, {t: "+", k: "in"}, {t: "Fit", k: "fit"}]
                delegate: Glass {
                    id: zb
                    required property var modelData
                    width: modelData.k === "fit" ? 44 : 32
                    height: 32
                    radius: 10
                    tint: zbMouse.containsMouse ? 0.9 : 0.55
                    Text {
                        anchors.centerIn: parent
                        text: zb.modelData.t
                        color: core.theme.text
                        font.pixelSize: 14
                    }
                    MouseArea {
                        id: zbMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (zb.modelData.k === "fit") {
                                root.fit()
                                return
                            }
                            root.userMoved = true
                            root.fitting = true
                            const ns = Math.max(0.25, Math.min(2.6, world.scale * (zb.modelData.k === "in" ? 1.25 : 0.8)))
                            const cx = canvas.width / 2, cy = canvas.height / 2
                            const wx = (cx - world.x) / world.scale, wy = (cy - world.y) / world.scale
                            world.scale = ns
                            world.x = cx - wx * ns
                            world.y = cy - wy * ns
                            fitDone.restart()
                        }
                    }
                }
            }
        }
    }

    // ---- classic fallback ---------------------------------------------------------------

    ItemGrid {
        anchors.fill: parent
        visible: !root.spatial
        host: root.host
        model: root.spatial ? null : root.model
        emptyText: root.emptyText
        styleOverride: root.styleOverride
        selectedId: root.selectedId
        collectionId: root.collectionId
        onItemClicked: id => root.itemClicked(id)
    }

    Text {
        anchors.centerIn: parent
        width: parent.width - 48
        visible: root.spatial && root.count === 0 && root.emptyText !== ""
        text: root.emptyText
        color: core.theme.subtext
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }
}
