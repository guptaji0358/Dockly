import QtQuick
import LH

// Radial command ring: sections on the inner ring, favourites on the outer one.
// Open with the orb, Ctrl+Space or the middle mouse button; Esc / click outside closes it.
Item {
    id: ring
    property var tabs: []
    property int current: 0
    property var host
    property bool opened: false
    signal picked(int index)

    readonly property var th: core.theme
    readonly property real p: shown
    property real shown: opened ? 1 : 0
    Behavior on shown {
        enabled: Fx.animated
        NumberAnimation { duration: 340; easing.type: opened ? Easing.OutBack : Easing.InCubic; easing.overshoot: 1.2 }
    }
    property string hint: ""

    visible: shown > 0.01
    focus: opened
    Keys.onEscapePressed: opened = false
    Keys.onPressed: event => {
        if (!opened)
            return
        event.accepted = true
        if (event.key === Qt.Key_Left || event.key === Qt.Key_Up)
            step(-1)
        else if (event.key === Qt.Key_Right || event.key === Qt.Key_Down)
            step(1)
        else if (event.key === Qt.Key_Tab)
            switchRing()
        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space)
            activateFocus()
        else
            event.accepted = false
    }

    // ---- focus: which node is aimed at (by keyboard, swipe / drag, or game controller) ----------------
    // fRing 0 = sections (inner ring), 1 = favourites (outer ring), -1 = nothing
    property int fRing: -1
    property int fIdx: -1
    readonly property int favCount: Math.min(favs.count, 14)
    onOpenedChanged: {
        if (opened) {
            fRing = 0
            fIdx = Math.max(0, current)
        } else {
            fRing = -1
            fIdx = -1
        }
        syncHint()
    }
    function countOf(r) { return r === 0 ? tabs.length : favCount }
    function syncHint() {
        if (fRing < 0 || fIdx < 0)
            hint = ""
        else if (fRing === 0)
            hint = tabs[fIdx] ? tabs[fIdx].full.toUpperCase() : ""
        else
            hint = favs.nameAt(fIdx).toUpperCase()
    }
    function focusAt(r, i) { fRing = r; fIdx = i; syncHint() }
    function step(d) {
        const r = fRing < 0 ? 0 : fRing
        const n = countOf(r)
        if (n === 0)
            return
        focusAt(r, fRing < 0 ? 0 : (fIdx + d + n) % n)
    }
    function switchRing() {
        const r = fRing === 1 ? 0 : (favCount > 0 ? 1 : 0)
        focusAt(r, Math.min(Math.max(fIdx, 0), countOf(r) - 1))
    }
    // the node whose direction from the centre is closest to the aimed direction (dx right, dy down)
    function aim(r, dx, dy) {
        const n = countOf(r)
        if (n === 0)
            return
        const a = Math.atan2(dy, dx)
        let best = 0, bestD = 10
        for (let i = 0; i < n; ++i) {
            const ang = r === 0 ? -Math.PI / 2 + 2 * Math.PI * i / n
                                : -Math.PI / 2 + Math.PI / n + 2 * Math.PI * i / n
            let d = Math.abs(a - ang) % (2 * Math.PI)
            if (d > Math.PI)
                d = 2 * Math.PI - d
            if (d < bestD) { bestD = d; best = i }
        }
        focusAt(r, best)
    }
    function activateFocus() {
        if (fRing < 0 || fIdx < 0)
            return
        const r = fRing, i = fIdx
        close()
        if (r === 0)
            picked(i)
        else if (host)
            host.launch(favs.idAt(i))
    }

    // Game controller. XInput is global, so it only acts while Dockly's own window is in front -
    // a pad used in a game must never open menus here.
    readonly property bool padOn: core.settings.values.gamepadEnabled && host && host.active
    Connections {
        target: core.gamepad
        enabled: ring.padOn
        function onChord() { ring.opened = !ring.opened }
        function onStick(x, y) {
            if (!ring.opened || Math.hypot(x, y) < 0.5)
                return
            ring.aim(ring.fRing < 0 ? 0 : ring.fRing, x, y)
        }
        function onPressed(button) {
            if (!ring.opened)
                return
            if (button === "left" || button === "up") ring.step(-1)
            else if (button === "right" || button === "down") ring.step(1)
            else if (button === "prev") ring.focusAt(0, Math.min(Math.max(ring.fIdx, 0), ring.countOf(0) - 1))
            else if (button === "next" && ring.favCount > 0) ring.focusAt(1, Math.min(Math.max(ring.fIdx, 0), ring.countOf(1) - 1))
            else if (button === "accept") ring.activateFocus()
            else if (button === "back" || button === "menu") ring.close()
        }
    }

    readonly property real cx: width / 2
    readonly property real cy: height / 2
    readonly property real r1: Math.min(width, height) * 0.25
    readonly property real r2: r1 + 112

    ItemModel { id: favs; mode: LH.ModeFavorites }

    function close() { opened = false }

    // backdrop
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(ring.th.bg.r, ring.th.bg.g, ring.th.bg.b, 0.82 * ring.p)
        // A plain click closes the ring. Press, drag and release is a pie-menu gesture, for mouse and touch alike:
        // the node in the dragged direction lights up (a short drag = a section, a long one = a favourite)
        // and is opened on release.
        MouseArea {
            anchors.fill: parent
            property point start
            property bool swiped: false
            onPressed: m => { start = Qt.point(m.x, m.y); swiped = false }
            onPositionChanged: m => {
                if (!pressed)
                    return
                const dx = m.x - start.x, dy = m.y - start.y
                const len = Math.hypot(dx, dy)
                if (len < 36)
                    return
                swiped = true
                ring.aim(len > 150 && ring.favCount > 0 ? 1 : 0, dx, dy)
            }
            onReleased: if (swiped) ring.activateFocus()
            onClicked: if (!swiped) ring.close()
        }
    }

    // guide circles
    Repeater {
        model: [ring.r1, ring.r2]
        delegate: Rectangle {
            required property real modelData
            required property int index
            readonly property real rr: modelData * ring.p
            x: ring.cx - rr; y: ring.cy - rr; width: rr * 2; height: rr * 2
            radius: rr
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(ring.th.accent.r, ring.th.accent.g, ring.th.accent.b, index === 0 ? 0.35 : 0.16)
            opacity: ring.p
        }
    }

    // core
    Item {
        x: ring.cx - 64; y: ring.cy - 64; width: 128; height: 128
        scale: 0.5 + 0.5 * ring.p
        opacity: ring.p
        Rectangle {
            anchors.fill: parent
            radius: 64
            color: Qt.rgba(ring.th.accent.r, ring.th.accent.g, ring.th.accent.b, 0.16)
            border.width: 2
            border.color: ring.th.accent
        }
        Icon { anchors.horizontalCenter: parent.horizontalCenter; y: 26; name: "logo"; size: 40; color: ring.th.accent }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 76
            width: 110
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: ring.hint !== "" ? ring.hint : "DOCKLY"
            color: ring.th.text
            font.pixelSize: 12
            font.letterSpacing: 2
            font.weight: Font.DemiBold
        }
    }

    // sections
    Repeater {
        model: ring.tabs
        delegate: Item {
            id: node
            required property int index
            required property var modelData
            readonly property real ang: -Math.PI / 2 + 2 * Math.PI * index / ring.tabs.length
            readonly property bool on: ring.current === index
            readonly property bool aimed: ring.fRing === 0 && ring.fIdx === index
            width: 76; height: 76
            x: ring.cx + Math.cos(ang) * ring.r1 * ring.p - width / 2
            y: ring.cy + Math.sin(ang) * ring.r1 * ring.p - height / 2
            opacity: Math.min(1, ring.p * 1.4)
            scale: (0.4 + 0.6 * ring.p) * (nm.containsMouse || aimed ? 1.14 : 1)
            Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 120 } }

            Rectangle {
                anchors.fill: parent
                radius: 38
                color: node.on ? ring.th.accent : Qt.rgba(ring.th.surface.r, ring.th.surface.g, ring.th.surface.b, 0.92)
                border.width: nm.containsMouse || node.aimed ? 3 : 1
                border.color: nm.containsMouse || node.on || node.aimed ? ring.th.accent : ring.th.border
            }
            Icon {
                anchors.centerIn: parent
                name: node.modelData.icon
                size: 30
                color: node.on ? "white" : ring.th.text
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.bottom
                anchors.topMargin: 6
                text: node.modelData.name
                color: nm.containsMouse || node.on || node.aimed ? ring.th.accent : ring.th.subtext
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            MouseArea {
                id: nm
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onContainsMouseChanged: ring.hint = containsMouse ? node.modelData.full.toUpperCase() : ""
                onClicked: { ring.close(); ring.picked(node.index) }
            }
        }
    }

    // favourites: tap to launch
    Repeater {
        model: favs
        delegate: Item {
            id: fav
            required property int index
            required property string itemId
            required property string name
            required property string iconSource
            readonly property int total: Math.min(favs.count, 14)
            readonly property real ang: -Math.PI / 2 + Math.PI / total + 2 * Math.PI * index / Math.max(1, total)
            readonly property bool aimed: ring.fRing === 1 && ring.fIdx === index
            visible: index < 14
            width: 54; height: 54
            x: ring.cx + Math.cos(ang) * ring.r2 * ring.p - width / 2
            y: ring.cy + Math.sin(ang) * ring.r2 * ring.p - height / 2
            opacity: Math.min(1, ring.p * 1.4)
            scale: (0.4 + 0.6 * ring.p) * (fm.containsMouse || aimed ? 1.2 : 1)
            Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 120 } }

            Rectangle {
                anchors.fill: parent
                radius: 27
                color: Qt.rgba(ring.th.surface.r, ring.th.surface.g, ring.th.surface.b, 0.92)
                border.width: fav.aimed ? 3 : 1
                border.color: fm.containsMouse || fav.aimed ? ring.th.favorite : ring.th.border
            }
            Image {
                anchors.centerIn: parent
                width: 30; height: 30
                source: fav.iconSource
                sourceSize: Qt.size(60, 60)
                fillMode: Image.PreserveAspectFit
                mipmap: true
            }
            MouseArea {
                id: fm
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onContainsMouseChanged: ring.hint = containsMouse ? fav.name.toUpperCase() : ""
                onClicked: { ring.close(); ring.host.launch(fav.itemId) }
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 26
        opacity: ring.p
        text: "Sections  ·  inner ring        Favourites  ·  outer ring        drag or ← → + Enter to pick        Esc to close"
        color: ring.th.subtext
        font.pixelSize: 12
        font.letterSpacing: 1
    }
}
