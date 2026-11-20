import QtQuick
import QtQuick.Controls
import QtQuick.Window
import LH

// The slim launcher strip: icons only, docked to a screen edge, no title bar.
// Collapsed it is a thin accent line on the edge; the pointer reaching the edge slides it open,
// and launching something (or leaving it alone) slides it away again.
// Dragged into a top or bottom corner it bends into an L: the icons continue on the second edge (a second window).
// Or split across the left and right edges (right-click menu): half the icons on each side.
Window {
    id: dock

    readonly property var th: core.theme
    // bottom is where the taskbar lives: it exists only as one leg of a two-edge dock (a bottom corner), never as a strip of its own
    readonly property string edgeRaw: core.settings.values.dockEdge
    readonly property string edge: edgeRaw === "bottom" && !bent ? "right" : edgeRaw
    function horiz(e) { return e === "top" || e === "bottom" }
    readonly property bool vertical: !horiz(edge)
    readonly property bool autoHide: core.settings.values.dockAutoHide && !dockPinned
    readonly property rect area: workArea

    readonly property int cell: 52          // one icon slot
    readonly property int iconSize: 40
    readonly property int thickness: 64     // across the strip
    readonly property int handle: 8         // what stays on the edge while collapsed
    readonly property int margin: 6         // gap between the strip and the screen edge
    readonly property int gripSize: 18      // the drag handle at one end of the strip
    readonly property int cornerR: 90       // pointer this close to a top corner while dragging = bend into an L

    // where along its edge the dock sits (fraction of the work area, set by dragging the grip); centred by default
    readonly property real frac: {
        const f = parseFloat(core.settings.values.dockPos)
        return isNaN(f) ? 0.5 : Math.max(0, Math.min(1, f))
    }

    // Two legs: the dock is shared by two edges. Either an L in a top corner (the top edge plus the left or right
    // one), or split across both sides (left + right, each leg centred on its own edge). The first `split` icons
    // sit on the main edge, the rest on the second edge.
    property int splitDrag: -1 // while the split grip is dragged
    readonly property string edge2Raw: String(core.settings.values.dockEdge2)
    readonly property bool edge2Side: edge2Raw === "left" || edge2Raw === "right"
    readonly property bool pairOk: horiz(edgeRaw) ? edge2Side
                                                  : (horiz(edge2Raw) || edge2Side && edge2Raw !== edgeRaw)
    // the user's own count; until they pick one, a freshly chosen second edge takes half the icons
    readonly property int splitRaw: splitDrag >= 0 ? splitDrag : (parseInt(core.settings.values.dockSplit) || (pairOk ? Math.floor(total / 2) : 0))
    onBentChanged: if (bent && !parseInt(core.settings.values.dockSplit)) core.settings.set("dockSplit", String(split))
    readonly property bool bent: pairOk && splitRaw > 0 && total >= 2
    readonly property string edge2: bent ? edge2Raw : ""
    readonly property bool sides: bent && !horiz(edge) && !horiz(edge2) // left + right: two independent strips
    readonly property int split: Math.max(1, Math.min(splitRaw, total - 1))
    readonly property int total: (catMode || view === 1) ? cats.length : itemModel.count

    // drag-to-place: the dock stays put; ghost strips show the spot it will snap to on release
    property bool placing: false
    property string placeEdge: "right"
    property string placeEdge2: ""
    property bool placeBent: false
    property int placeSplit: 1
    property real placeFrac: 0.5
    property bool placeKeepSides: false // the dock was split across both sides when the drag began
    readonly property bool placeSides: placeBent && !horiz(placeEdge) && !horiz(placeEdge2)
    readonly property bool busy: placing || splitDrag >= 0

    function dragMove(g) {
        const toLeft = g.x - area.x
        const toRight = area.x + area.width - g.x
        const toTop = g.y - area.y
        const toBottom = area.y + area.height - g.y
        const toSide = Math.min(toLeft, toRight)
        const toHz = Math.min(toTop, toBottom)
        if (total >= 2 && toHz < cornerR && toSide < cornerR) {
            const side = toLeft < toRight ? "left" : "right"
            const hz = toTop <= toBottom ? "top" : "bottom"
            placeBent = true
            placeEdge = toHz <= toSide ? hz : side
            placeEdge2 = toHz <= toSide ? side : hz
            placeSplit = Math.max(1, Math.min(splitRaw > 0 ? splitRaw : Math.floor(total / 2), total - 1))
            return
        }
        if (placeKeepSides) {
            // both halves stay split: the leg being dragged follows the pointer to its side, the other takes the opposite
            const e = toLeft <= toRight ? "left" : "right"
            let f = Math.max(0, Math.min(1, (g.y - area.y) / area.height))
            if (Math.abs(f - 0.5) < 0.02) f = 0.5
            placeBent = true
            placeEdge = e
            placeEdge2 = e === "left" ? "right" : "left"
            placeSplit = Math.max(1, Math.min(splitRaw, total - 1))
            placeFrac = f
            return
        }
        placeBent = false
        placeEdge2 = ""
        let e = "left"
        let d = toLeft
        if (toRight < d) { e = "right"; d = toRight }
        if (toTop < d) e = "top"
        let f = e === "top" ? toLeft / area.width : (g.y - area.y) / area.height
        f = Math.max(0, Math.min(1, f))
        if (Math.abs(f - 0.5) < 0.02) f = 0.5 // magnet to the middle
        placeEdge = e
        placeFrac = f
    }
    function beginPlacing(g) {
        placeEdge = edge
        placeEdge2 = edge2
        placeBent = bent
        placeSplit = bent ? split : 1
        placeFrac = frac
        placeKeepSides = sides
        placing = true
        dragMove(g)
    }
    function endPlacing(commit) {
        placing = false
        if (commit) {
            core.settings.set("dockEdge", placeEdge)
            core.settings.set("dockEdge2", placeBent ? placeEdge2 : "")
            core.settings.set("dockSplit", placeBent ? String(placeSplit) : "")
            if (!placeBent || placeSides)
                core.settings.set("dockPos", placeFrac.toFixed(4))
        }
    }
    // split the dock across the left and right edges, half the icons on each
    function splitSides() {
        if (total < 2)
            return
        core.settings.set("dockEdge", "left")
        core.settings.set("dockEdge2", "right")
        core.settings.set("dockSplit", String(Math.ceil(total / 2)))
    }
    // the divider grip: how many icons this leg (p) holds follows the pointer's distance from the leg's start
    function splitMove(g, p) {
        const e = p === 0 ? edge : edge2
        const o = p === 0 ? edge2 : edge
        let dist
        if (sides) // each leg is centred on its own edge, so its length grows both ways from the centre
            dist = 2 * Math.abs(g.y - (area.y + area.height * frac))
        else if (horiz(e)) // an L: the leg starts in the corner on the side named by the other edge
            dist = o === "right" ? area.x + area.width - g.x : g.x - area.x
        else
            dist = o === "bottom" ? area.y + area.height - thickness - g.y : g.y - (area.y + thickness)
        const slots = Math.round((dist - 16 - 2 * gripSize) / cell) // icons plus the header or gear
        const mine = Math.max(1, Math.min(total - 1, slots - (p === 0 && !catMode ? 1 : 0) - (p === 1 ? 1 : 0)))
        splitDrag = p === 0 ? mine : total - mine
    }
    function endSplit(commit) {
        if (commit && splitDrag >= 0)
            core.settings.set("dockSplit", String(splitDrag))
        splitDrag = -1
    }
    function straighten() {
        if (edgeRaw === "bottom")
            core.settings.set("dockEdge", "right")
        core.settings.set("dockEdge2", "")
        core.settings.set("dockSplit", "")
    }

    // ---- geometry of the strip(s) ----------------------------------------
    function countOf(p, b, sp) { return !b ? total : (p === 0 ? sp : total - sp) }
    function lenOf(p, b, sp, e, corner) {
        const slots = countOf(p, b, sp) + (p === 0 && !catMode ? 1 : 0) + (!b || p === 1 ? 1 : 0) // header, gear
        const room = (horiz(e) ? area.width : area.height) - 80 - (corner ? thickness : 0)
        const grips = b ? 2 : 1 // a two-edge leg has a move grip at one end and a divider at the other
        return Math.max(cell + 16 + grips * gripSize, Math.min(slots * cell + 16 + grips * gripSize, room))
    }
    // window rectangle of part p (0 = main edge, 1 = second edge) when `thick` wide
    function rectOf(p, b, sp, eP, eO, f, thick) {
        const hz = horiz(eP)
        const corner = b && hz !== horiz(eO) // an L; left + right legs are plain, independent strips
        const len = lenOf(p, b, sp, eP, corner)
        const w = hz ? len : thick
        const h = hz ? thick : len
        const clamp = (v, lo, hi) => Math.max(lo, Math.min(v, hi))
        const bottom = area.y + area.height
        if (corner) {
            const side = hz ? eO : eP
            const hEdge = hz ? eP : eO
            const x = side === "right" ? area.x + area.width - w : area.x
            // the horizontal strip owns the corner; the vertical one starts beside it
            const y = hz ? (hEdge === "top" ? area.y : bottom - h)
                         : (hEdge === "top" ? area.y + thickness : bottom - thickness - h)
            return Qt.rect(x, y, w, h)
        }
        const x = eP === "right" ? area.x + area.width - w
                : eP === "left" ? area.x
                : clamp(area.x + area.width * f - w / 2, area.x, area.x + area.width - w)
        const y = eP === "top" ? area.y
                : eP === "bottom" ? bottom - h
                : clamp(area.y + area.height * f - h / 2, area.y, bottom - h)
        return Qt.rect(x, y, w, h)
    }
    readonly property real current: handle + (thickness - handle) * openness
    readonly property rect r0: rectOf(0, bent, split, edge, edge2, frac, current)
    readonly property rect r1: rectOf(1, bent, split, edge2, edge, frac, current)

    property bool shown: true
    property real openness: shown ? 1 : 0
    Behavior on openness { NumberAnimation { duration: Fx.animated ? 160 : 0; easing.type: Easing.OutCubic } }

    function reveal() {
        shown = true
        hideTimer.interval = 3000 // opened without the pointer (hotkey, tray): give it a moment
        hideTimer.restart()
    }
    function toggle() {
        if (shown && openness > 0.5 && autoHide)
            shown = false
        else
            reveal()
    }
    readonly property bool menuOpen: tileMenu.visible || dockMenu.visible
    readonly property bool anyHover: face0.hovered || face1.hovered

    // Three views, always reached the same way: 0 = favourites (home, or everything until there are any),
    // 1 = the category picker, 2 = the items of one category (or the members of one collection)
    readonly property bool catMode: core.settings.values.dockMode === "categories"
    property int flyCat: -1 // category whose grid is open beside the dock (categories mode)
    property int view: 0
    property int browseCat: 0
    property bool inCollection: false
    property string collId

    // the five built-in categories, then one entry per site that has links (100 + n = the n-th site)
    readonly property var baseCats: [
        {name: "Apps", cat: 0, icon: "software", col: "#4C8DFF"},
        {name: "Documents", cat: 1, icon: "documents", col: "#FFA94D"},
        {name: "Entertainment", cat: 2, icon: "entertainment", col: "#F06595"},
        {name: "Web", cat: 3, icon: "web", col: "#3DDC97"},
        {name: "Collections", cat: 4, icon: "collections", col: "#A78BFA"}
    ]
    property var siteCats: core.sites.dockEntries()
    readonly property var cats: baseCats.concat(siteCats)
    Connections {
        target: core
        function onItemsChanged() { Qt.callLater(dock.refreshSites) }
    }
    Connections {
        target: core.sites
        function onChanged() { Qt.callLater(dock.refreshSites) }
    }
    function refreshSites() {
        siteCats = core.sites.dockEntries()
        if (flyCat >= 0 && !cats.some(c => c.cat === flyCat)) flyCat = -1
    }

    ItemModel { id: favs; mode: LH.ModeFavorites }
    ItemModel { id: everything }
    ItemModel {
        id: browse
        mode: dock.inCollection ? LH.ModeMembers : LH.ModeCategory
        category: dock.browseCat
        collectionId: dock.collId
    }
    readonly property var homeModel: favs.count > 0 ? favs : everything
    readonly property var itemModel: view === 2 ? browse : homeModel

    function back() {
        if (inCollection)
            inCollection = false
        else
            view = 1
    }
    function headerAction() {
        if (view === 0) view = 1
        else if (view === 1) view = 0
        else back()
    }
    // the dock always comes back on favourites
    onOpennessChanged: if (openness === 0) { view = 0; inCollection = false; flyCat = -1 }

    ItemModel { id: flyModel; mode: LH.ModeCategory; category: Math.max(dock.flyCat, 0) }

    // Full-screen apps: the dock leaves the screen entirely (no edge line over a game's or editor's UI) and
    // only slides back when the pointer rests on its edge. See DockGuard.
    readonly property bool guarded: core.settings.values.dockHideFullscreen && !dockPinned
    visible: (core.settings.values.dockEnabled && dockAllowed && (!guarded || dockGuard.dockVisible)) || dockPinned
    // never takes keyboard focus, so using the dock can't pull focus away from the program you are in
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus

    // where the pointer must rest to open the collapsed dock: exactly the visible line
    readonly property rect edgeZone: rectOf(0, bent, split, edge, edge2, frac, handle)
    // The collapsed line is only a marker: every click and hover goes through it to the program below
    // (browser tabs, Explorer tabs, scroll bars). Resting the pointer on it for a moment opens the dock.
    readonly property bool collapsed: !shown && openness < 0.02 && !dockPinned
    Binding { target: dockGuard; property: "clickThrough"; value: dock.collapsed }
    Connections {
        target: dockGuard
        function onEdgeDwelled() { dock.reveal() }
    }
    // the dock fully open; while a flyout or menu is open the whole screen counts so it doesn't vanish under them
    readonly property rect dockZone: {
        if (flyCat >= 0 || menuOpen)
            return area
        const a = rectOf(0, bent, split, edge, edge2, frac, thickness)
        return a // two legs are guarded as two zones, not as one box spanning the screen
    }
    // the other leg (an L or a left + right dock): its own line, its own zone
    readonly property rect edgeZone2: bent ? rectOf(1, bent, split, edge2, edge, frac, handle) : Qt.rect(0, 0, 0, 0)
    readonly property rect dockZone2: {
        if (!bent)
            return Qt.rect(0, 0, 0, 0)
        return (flyCat >= 0 || menuOpen) ? area : rectOf(1, bent, split, edge2, edge, frac, thickness)
    }
    // Idle pointer: the collapsed line hides itself (setting dockIdleHide, in seconds; 0 = never).
    readonly property bool lineIdle: dock.collapsed && dockGuard.idle
    // 1 = tucked into the edge; the line peeks out gradually as the pointer approaches, then springs back fully
    property real lineGone: lineIdle ? 1 - dockGuard.proximity * 0.85 : 0
    Behavior on lineGone {
        NumberAnimation {
            duration: Fx.animated ? (!lineIdle ? 260 : dockGuard.proximity > 0 ? 160 : 650) : 0
            easing.type: lineIdle && dockGuard.proximity <= 0 ? Easing.InOutCubic : Easing.OutBack
            easing.overshoot: 1.4
        }
    }
    Binding { target: dockGuard; property: "idleAfter"; value: core.settings.values.dockIdleHide > 0 ? Math.max(2, core.settings.values.dockIdleHide) * 1000 : 0 }
    Binding { target: dockGuard; property: "calmApps"; value: String(core.settings.values.dockCalmApps).split(",") }
    Binding { target: dockGuard; property: "enabled"; value: dock.guarded }
    Binding { target: dockGuard; property: "window"; value: dock }
    Binding { target: dockGuard; property: "edgeZone"; value: dock.edgeZone }
    Binding { target: dockGuard; property: "dockZone"; value: dock.dockZone }
    Binding { target: dockGuard; property: "window2"; value: dock.bent ? dock2 : null }
    Binding { target: dockGuard; property: "edgeZone2"; value: dock.edgeZone2 }
    Binding { target: dockGuard; property: "dockZone2"; value: dock.dockZone2 }
    color: "transparent"
    title: "Dockly dock"

    x: r0.x
    y: r0.y
    width: r0.width
    height: r0.height

    // how long the pointer may be off the open dock before it slides away (setting dockCollapseDelay, ms)
    readonly property int collapseDelay: core.settings.values.dockCollapseDelay

    onAutoHideChanged: {
        if (!autoHide)
            shown = true
        else
            hideTimer.restart()
    }
    Component.onCompleted: hideTimer.interval = 2500 // show briefly on startup so you can see where it lives
    onVisibleChanged: if (visible) reveal()

    Timer {
        id: hideTimer
        interval: dock.collapseDelay
        onTriggered: {
            if (dock.autoHide && !dock.anyHover && !dock.menuOpen && !dock.busy && !face0.dragging && !face1.dragging
                    && dock.flyCat < 0)
                dock.shown = false
            interval = dock.collapseDelay
        }
    }

    Face {
        id: face0
        part: 0
        anchors.fill: parent
    }

    // the second leg: keeps its own thin line on screen when collapsed, like the first
    Window {
        id: dock2
        flags: dock.flags
        color: "transparent"
        title: "Dockly dock 2"
        visible: dock.visible && dock.bent
        x: dock.r1.x
        y: dock.r1.y
        width: dock.r1.width
        height: dock.r1.height

        Face {
            id: face1
            part: 1
            anchors.fill: parent
        }
    }

    // one leg of the dock: the thin edge line while collapsed, the strip of icons when open
    component Face: Item {
        id: face
        required property int part
        readonly property string myEdge: part === 0 ? dock.edge : dock.edge2
        readonly property string otherEdge: part === 0 ? dock.edge2 : dock.edge
        readonly property bool vert: !dock.horiz(myEdge)
        readonly property int from: part === 0 ? 0 : dock.split
        readonly property int to: part === 0 && dock.bent ? dock.split : 100000
        readonly property bool head: part === 0
        readonly property bool gear: !dock.bent || part === 1
        // icons start in the corner: a horizontal leg in a right-hand corner runs right to left, a vertical one in a bottom corner bottom to top
        readonly property bool flip: dock.bent && (vert ? otherEdge === "bottom" : otherEdge === "right")
        readonly property bool rtl: flip && !vert
        readonly property bool btt: flip && vert
        // the move grip sits at the start of the leg (the corner); the divider grip at its free end
        readonly property bool hasMax: flip || dock.bent
        property alias hovered: hov.hovered
        property alias dragging: drops.containsDrag
        readonly property int topPad: 6 + (vert ? dock.gripSize : 0)
        readonly property int bottomPad: 6 + (vert && hasMax ? dock.gripSize : 0)
        readonly property int leftPad: 6 + (!vert ? dock.gripSize : 0)
        readonly property int rightPad: 6 + (!vert && hasMax ? dock.gripSize : 0)
        clip: true

        HoverHandler {
            id: hov
            onHoveredChanged: {
                if (hovered) {
                    dock.shown = true
                    hideTimer.stop()
                } else {
                    tip.label = ""
                    hideTimer.interval = dock.collapseDelay
                    hideTimer.restart()
                    flyTimer.restart()
                }
            }
        }

        // drop programs, files, folders or links here to add them (and pin them to the dock)
        DropArea {
            id: drops
            anchors.fill: parent
            onEntered: drag => {
                drag.accepted = drag.hasUrls || drag.hasText || drag.formats.indexOf("application/x-dockly-item") >= 0
                dock.shown = true
                hideTimer.stop()
            }
            onExited: { hideTimer.interval = dock.collapseDelay; hideTimer.restart() }
            onDropped: drop => {
                // a card dragged out of the main window: pin it to the dock
                const cardId = drop.getDataAsString("application/x-dockly-item")
                if (cardId !== "") {
                    drop.accept()
                    core.setFavorite(cardId, true)
                    core.message("Pinned to the dock.")
                    return
                }
                const items = []
                if (drop.hasUrls)
                    for (const u of drop.urls)
                        items.push(u.toString())
                else if (drop.hasText)
                    items.push(drop.text.trim())
                drop.accept()
                core.addDropped(items, -1, true)
            }
        }

        // collapsed: a thin line so you can see (and hit) the edge; after a while without touching the
        // mouse or trackpad it slips into the screen edge, and comes straight back when the pointer moves
        Rectangle {
            anchors.fill: parent
            radius: 3
            color: dock.th.accent
            opacity: 0.95 * (1 - dock.openness) * Math.min(1, Math.max(0, 1 - dock.lineGone))
            visible: opacity > 0.01
            // slides into the edge and shrinks toward its middle; the reverse when the pointer comes near
            transform: [
                Scale {
                    origin.x: width / 2
                    origin.y: height / 2
                    xScale: face.vert ? 1 : 1 - 0.5 * Math.min(1, dock.lineGone)
                    yScale: face.vert ? 1 - 0.5 * Math.min(1, dock.lineGone) : 1
                },
                Translate {
                    x: dock.lineGone * dock.handle * (face.myEdge === "right" ? 1 : face.myEdge === "left" ? -1 : 0)
                    y: dock.lineGone * dock.handle * (face.myEdge === "top" ? -1 : face.myEdge === "bottom" ? 1 : 0)
                }
            ]
        }

        // the strip itself, pinned to the screen edge so it slides in and out with the window
        Rectangle {
            id: strip
            width: face.vert ? dock.thickness - dock.margin : parent.width
            height: face.vert ? parent.height : dock.thickness - dock.margin
            x: face.myEdge === "right" ? parent.width - width - dock.margin : (face.myEdge === "left" ? dock.margin : 0)
            y: face.myEdge === "top" ? dock.margin : (face.myEdge === "bottom" ? parent.height - height - dock.margin : 0)
            radius: 18
            color: Qt.alpha(dock.th.surface, core.settings.values.dockOpacity / 100)
            border.width: 0
            opacity: dock.openness

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: dockMenu.popup()
            }

            // Two grips per leg. Dots (the corner end): press and drag along the borders to move the dock, into a corner to bend it;
            // double-click centres it. Bars (the free end): slide to decide how many icons this edge holds; double-click straightens.
            Grip { face: face; atMax: face.flip }
            Grip { face: face; divider: true; atMax: !face.flip; visible: dock.bent }

            Component { id: headC; Head {} }
            Component { id: gearC; Gear {} }
            Component { id: gearCatC; Gear { onHoveredChanged: if (hovered) dock.flyCat = -1 } }

            // favourites / one category's items
            ListView {
                id: list
                anchors.fill: parent
                anchors.topMargin: face.topPad
                anchors.bottomMargin: face.bottomPad
                anchors.leftMargin: face.leftPad
                anchors.rightMargin: face.rightPad
                visible: !dock.catMode && dock.view !== 1
                orientation: face.vert ? ListView.Vertical : ListView.Horizontal
                layoutDirection: face.rtl ? Qt.RightToLeft : Qt.LeftToRight
                verticalLayoutDirection: face.btt ? ListView.BottomToTop : ListView.TopToBottom
                interactive: contentWidth > width || contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: dock.itemModel
                header: face.head ? headC : null
                footer: face.gear ? gearC : null

                delegate: Slot {
                    id: slot
                    required property int index
                    required property string itemId
                    required property string name
                    required property string iconSource
                    required property int category
                    required property bool running
                    readonly property bool mine: index >= face.from && index < face.to
                    visible: mine
                    width: mine ? dock.cell - 4 : 0
                    height: mine ? dock.cell - 4 : 0
                    tipText: name
                    onActivated: {
                        if (dock.view === 2 && category === LH.Collection) {
                            dock.collId = itemId
                            dock.inCollection = true
                            return
                        }
                        if (core.launch(itemId) === LH.TargetMissing)
                            tray.openMain(itemId)
                        else if (dock.autoHide)
                            dock.shown = false
                    }
                    onRightClicked: {
                        tileMenu.itemId = itemId
                        tileMenu.pinned = dock.view === 0 && favs.count > 0
                        tileMenu.popup()
                    }
                    Image {
                        anchors.centerIn: parent
                        width: dock.iconSize
                        height: dock.iconSize
                        source: slot.iconSource
                        sourceSize: Qt.size(dock.iconSize * 2, dock.iconSize * 2)
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        scale: slot.pressed ? 0.88 : (slot.hovered ? 1.08 : 1)
                        Behavior on scale { NumberAnimation { duration: 90 } }
                    }
                    // open program: a small dot at the bottom of its slot
                    Rectangle {
                        visible: slot.running && core.settings.values.showRunning
                        width: 5; height: 5; radius: 2.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        color: dock.th.accent
                    }
                }
            }

            // categories mode: five big colour-coded icons; hovering one opens a grid of its apps beside the dock
            ListView {
                id: catStrip
                anchors.fill: parent
                anchors.topMargin: face.topPad
                anchors.bottomMargin: face.bottomPad
                anchors.leftMargin: face.leftPad
                anchors.rightMargin: face.rightPad
                visible: dock.catMode
                orientation: list.orientation
                layoutDirection: list.layoutDirection
                verticalLayoutDirection: list.verticalLayoutDirection
                interactive: contentWidth > width || contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: dock.cats
                footer: face.gear ? gearCatC : null

                delegate: Slot {
                    id: stripSlot
                    required property int index
                    required property var modelData
                    readonly property bool mine: index >= face.from && index < face.to
                    visible: mine
                    width: mine ? dock.cell - 4 : 0
                    height: mine ? dock.cell - 4 : 0
                    onHoveredChanged: if (hovered) dock.openFly(modelData.cat, stripSlot)
                    onActivated: dock.openFly(modelData.cat, stripSlot)
                    Rectangle {
                        anchors.centerIn: parent
                        width: dock.iconSize
                        height: dock.iconSize
                        radius: 12
                        color: Qt.alpha(stripSlot.modelData.col, dock.flyCat === stripSlot.modelData.cat ? 0.4 : 0.22)
                        scale: stripSlot.pressed ? 0.88 : (stripSlot.hovered ? 1.08 : 1)
                        Behavior on scale { NumberAnimation { duration: 90 } }
                        SiteIcon {
                            anchors.centerIn: parent
                            image: stripSlot.modelData.image ?? ""
                            name: stripSlot.modelData.icon
                            size: 24
                            color: stripSlot.modelData.col
                        }
                    }
                }
            }

            // the category picker: five big colour-coded icons
            ListView {
                id: catList
                anchors.fill: parent
                anchors.topMargin: face.topPad
                anchors.bottomMargin: face.bottomPad
                anchors.leftMargin: face.leftPad
                anchors.rightMargin: face.rightPad
                visible: !dock.catMode && dock.view === 1
                orientation: list.orientation
                layoutDirection: list.layoutDirection
                verticalLayoutDirection: list.verticalLayoutDirection
                interactive: contentWidth > width || contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: dock.cats
                header: face.head ? headC : null
                footer: face.gear ? gearC : null

                delegate: Slot {
                    id: catSlot
                    required property int index
                    required property var modelData
                    readonly property bool mine: index >= face.from && index < face.to
                    visible: mine
                    width: mine ? dock.cell - 4 : 0
                    height: mine ? dock.cell - 4 : 0
                    tipText: modelData.name
                    onActivated: {
                        dock.browseCat = modelData.cat
                        dock.inCollection = false
                        dock.view = 2
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: dock.iconSize
                        height: dock.iconSize
                        radius: 12
                        color: Qt.alpha(catSlot.modelData.col, 0.22)
                        scale: catSlot.pressed ? 0.88 : (catSlot.hovered ? 1.08 : 1)
                        Behavior on scale { NumberAnimation { duration: 90 } }
                        SiteIcon {
                            anchors.centerIn: parent
                            image: catSlot.modelData.image ?? ""
                            name: catSlot.modelData.icon
                            size: 24
                            color: catSlot.modelData.col
                        }
                    }
                }
            }
        }
    }

    // a drag handle at one end of a leg: dots move the dock, bars slide the divider between the two edges
    component Grip: Item {
        id: gp
        property var face
        property bool divider: false
        property bool atMax: false
        width: face.vert ? parent.width : dock.gripSize + 6
        height: face.vert ? dock.gripSize + 6 : parent.height
        x: !face.vert && atMax ? parent.width - width : 0
        y: face.vert && atMax ? parent.height - height : 0
        readonly property color tint: gripMouse.containsMouse || dock.busy ? dock.th.accent : dock.th.subtext
        Flow {
            visible: !gp.divider
            anchors.centerIn: parent
            flow: gp.face.vert ? Flow.LeftToRight : Flow.TopToBottom
            width: gp.face.vert ? 22 : 10
            height: gp.face.vert ? 10 : 22
            spacing: 4
            Repeater {
                model: 6
                Rectangle { width: 4; height: 4; radius: 2; color: gp.tint }
            }
        }
        Grid {
            visible: gp.divider
            anchors.centerIn: parent
            columns: gp.face.vert ? 1 : 2
            spacing: 3
            Repeater {
                model: 2
                Rectangle {
                    width: gp.face.vert ? 18 : 3
                    height: gp.face.vert ? 3 : 18
                    radius: 1.5
                    color: gp.tint
                }
            }
        }
        MouseArea {
            id: gripMouse
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            cursorShape: dock.busy ? Qt.ClosedHandCursor
                        : gp.divider ? (gp.face.vert ? Qt.SizeVerCursor : Qt.SizeHorCursor) : Qt.OpenHandCursor
            property point downAt
            function globalAt(m) { return mapToGlobal(m.x, m.y) }
            onPressed: m => downAt = globalAt(m)
            onPositionChanged: m => {
                if (!pressed)
                    return
                const g = globalAt(m)
                if (gp.divider)
                    dock.splitMove(g, gp.face.part)
                else if (dock.placing)
                    dock.dragMove(g)
                else if (Math.abs(g.x - downAt.x) + Math.abs(g.y - downAt.y) > 8)
                    dock.beginPlacing(g)
            }
            onReleased: gp.divider ? dock.endSplit(true) : (dock.placing ? dock.endPlacing(true) : 0)
            onCanceled: gp.divider ? dock.endSplit(false) : (dock.placing ? dock.endPlacing(false) : 0)
            onDoubleClicked: gp.divider ? dock.straighten() : core.settings.set("dockPos", "")
        }
    }

    // one icon slot: hover highlight, name tip, left click = activated, right click = rightClicked
    component Slot: Item {
        id: s
        property string tipText
        property alias hovered: m.containsMouse
        property alias pressed: m.pressed
        signal activated()
        signal rightClicked()
        default property alias content: holder.data
        width: dock.cell - 4
        height: dock.cell - 4

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: dock.th.cardHover
            opacity: m.containsMouse ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 90 } }
        }
        Item { id: holder; anchors.fill: parent }
        MouseArea {
            id: m
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            onEntered: dock.showTip(s.tipText, s)
            onExited: if (tip.label === s.tipText) tip.label = ""
            onClicked: mouse => {
                tip.label = ""
                if (mouse.button === Qt.RightButton)
                    s.rightClicked()
                else
                    s.activated()
            }
        }
    }

    // first slot, same place in every view: categories (from home), favourites (from the picker), back (inside a category)
    component Head: Slot {
        id: h
        tipText: dock.view === 0 ? "Categories" : (dock.view === 1 ? "Favourites" : "Back")
        onActivated: dock.headerAction()
        Grid {
            visible: dock.view === 0
            anchors.centerIn: parent
            columns: 2
            spacing: 4
            Repeater {
                model: 4
                Rectangle { width: 10; height: 10; radius: 3; color: h.hovered ? dock.th.text : dock.th.subtext }
            }
        }
        Icon {
            visible: dock.view === 1
            anchors.centerIn: parent
            name: "star"
            size: 26
            color: dock.th.favorite
        }
        Text {
            visible: dock.view === 2
            anchors.centerIn: parent
            text: "\u2039"
            font.pixelSize: 34
            color: h.hovered ? dock.th.text : dock.th.accent
        }
    }

    // last slot: opens the full window
    component Gear: Slot {
        id: g
        tipText: "Open Dockly"
        onActivated: {
            tray.openMain()
            if (dock.autoHide)
                dock.shown = false
        }
        Icon {
            anchors.centerIn: parent
            name: "settings"
            size: 22
            color: g.hovered ? dock.th.text : dock.th.subtext
        }
    }

    component DockMenu: Menu {
        popupType: Popup.Window
        palette.window: dock.th.surface
        palette.windowText: dock.th.text
        palette.text: dock.th.text
        palette.highlight: dock.th.cardHover
        palette.highlightedText: dock.th.text
        palette.mid: dock.th.border
        onVisibleChanged: if (!visible) { hideTimer.interval = dock.collapseDelay; hideTimer.restart() }
    }

    DockMenu {
        id: tileMenu
        property string itemId
        property bool pinned
        MenuItem { text: "Open"; onTriggered: core.launch(tileMenu.itemId) }
        MenuItem {
            readonly property bool applies: core.itemInfo(tileMenu.itemId).category === LH.Software
            text: "Run as Administrator"
            visible: applies
            height: applies ? implicitHeight : 0
            onTriggered: core.launchAsAdmin(tileMenu.itemId)
        }
        MenuItem {
            readonly property bool applies: core.itemInfo(tileMenu.itemId).category !== LH.Collection
            text: "Copy path"
            visible: applies
            height: applies ? implicitHeight : 0
            onTriggered: core.copyPath(tileMenu.itemId)
        }
        MenuItem { text: "Edit..."; onTriggered: tray.editInMain(tileMenu.itemId) }
        MenuItem {
            text: "Remove from dock"
            enabled: tileMenu.pinned
            onTriggered: core.setFavorite(tileMenu.itemId, false)
        }
    }

    DockMenu {
        id: dockMenu
        MenuItem { text: "Quick launcher (search)"; onTriggered: core.showLauncher() }
        MenuSeparator {}
        Menu {
            title: "Position"
            Repeater {
                model: [{t: "Right", v: "right"}, {t: "Left", v: "left"}, {t: "Top", v: "top"}]
                MenuItem {
                    required property var modelData
                    text: modelData.t
                    checkable: true
                    checked: dock.edge === modelData.v
                    onTriggered: {
                        core.settings.set("dockEdge", modelData.v)
                        dock.straighten()
                    }
                }
            }
        }
        MenuItem {
            text: "Split across left and right"
            enabled: dock.total >= 2 && !dock.sides
            onTriggered: dock.splitSides()
        }
        MenuItem {
            text: "Straighten (undo split / L-shape)"
            enabled: dock.bent
            onTriggered: dock.straighten()
        }
        MenuItem {
            text: "Auto-hide"
            checkable: true
            checked: core.settings.values.dockAutoHide
            onTriggered: core.settings.set("dockAutoHide", checked)
        }
        MenuSeparator {}
        MenuItem { text: "Open Dockly..."; onTriggered: tray.openMain() }
        MenuItem { text: "Quit Dockly"; onTriggered: Qt.quit() }
    }

    function openFly(cat, item) {
        tip.label = ""
        flyAnchor = item.mapToGlobal(item.width / 2, item.height / 2)
        flyWin = item.Window.window
        flyEdge = flyWin === dock2 ? dock.edge2 : dock.edge
        flyCat = cat
        flyTimer.stop()
    }
    property point flyAnchor
    property var flyWin: dock
    property string flyEdge: edge

    Timer {
        id: flyTimer
        interval: 280
        onTriggered: {
            if (!dock.anyHover && !flyHover.hovered && !dock.menuOpen) {
                dock.flyCat = -1
                hideTimer.restart()
            }
        }
    }

    // the grid of apps for the hovered category, in its own window since the dock itself is too slim to hold it
    Window {
        id: fly
        readonly property var cat: dock.cats.find(c => c.cat === dock.flyCat) ?? dock.cats[0]
        readonly property int cols: 4
        readonly property int tile: 88
        readonly property int rows: Math.max(1, Math.ceil(flyModel.count / cols))
        readonly property int gap: 4
        flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
        color: "transparent"
        visible: dock.flyCat >= 0 && dock.visible && dock.openness > 0.9
        width: cols * tile + 28
        height: Math.min(rows * tile + 74, dock.area.height - 40)
        x: dock.flyEdge === "right" ? dock.flyWin.x - width - gap
           : dock.flyEdge === "left" ? dock.flyWin.x + dock.flyWin.width + gap
           : Math.max(dock.area.x + 8, Math.min(dock.flyAnchor.x - width / 2, dock.area.x + dock.area.width - width - 8))
        y: dock.flyEdge === "top" ? dock.flyWin.y + dock.flyWin.height + gap
           : dock.flyEdge === "bottom" ? dock.flyWin.y - height - gap
           : Math.max(dock.area.y + 8, Math.min(dock.flyAnchor.y - height / 2, dock.area.y + dock.area.height - height - 8))

        HoverHandler {
            id: flyHover
            onHoveredChanged: if (hovered) flyTimer.stop(); else flyTimer.restart()
        }

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 0

            Row {
                x: 16
                y: 14
                spacing: 10
                Rectangle {
                    width: 26
                    height: 26
                    radius: 8
                    color: Qt.alpha(fly.cat.col, 0.25)
                    SiteIcon { anchors.centerIn: parent; image: fly.cat.image ?? ""; name: fly.cat.icon; size: 16; color: fly.cat.col }
                }
                Text {
                    height: 26
                    verticalAlignment: Text.AlignVCenter
                    text: fly.cat.name + "  ·  " + flyModel.count
                    color: dock.th.text
                    font.family: "Segoe UI"
                    font.pixelSize: 15
                    font.bold: true
                }
            }

            Text {
                visible: flyModel.count === 0
                anchors.centerIn: parent
                width: parent.width - 40
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: "Nothing here yet.\nAdd some from the Dockly window."
                color: dock.th.subtext
                font.family: "Segoe UI"
                font.pixelSize: 13
            }

            GridView {
                x: 14
                y: 54
                width: parent.width - 28
                height: parent.height - 68
                cellWidth: fly.tile
                cellHeight: fly.tile
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: flyModel

                delegate: Item {
                    id: cell
                    required property string itemId
                    required property string name
                    required property string iconSource
                    width: fly.tile
                    height: fly.tile

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        radius: 12
                        color: dock.th.cardHover
                        opacity: cellMouse.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 90 } }
                    }
                    Image {
                        id: cellIcon
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 8
                        width: 44
                        height: 44
                        source: cell.iconSource
                        sourceSize: Qt.size(88, 88)
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                        scale: cellMouse.pressed ? 0.9 : (cellMouse.containsMouse ? 1.08 : 1)
                        Behavior on scale { NumberAnimation { duration: 90 } }
                    }
                    Text {
                        anchors.top: cellIcon.bottom
                        anchors.topMargin: 5
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: fly.tile - 10
                        horizontalAlignment: Text.AlignHCenter
                        text: cell.name
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.WordWrap
                        color: dock.th.text
                        font.family: "Segoe UI"
                        font.pixelSize: 11
                    }
                    MouseArea {
                        id: cellMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mouse => {
                            if (mouse.button === Qt.RightButton) {
                                tileMenu.itemId = cell.itemId
                                tileMenu.pinned = false
                                tileMenu.popup()
                                return
                            }
                            if (core.launch(cell.itemId) === LH.TargetMissing)
                                tray.openMain(cell.itemId)
                            dock.flyCat = -1
                            if (dock.autoHide)
                                dock.shown = false
                        }
                    }
                }
            }
        }
    }

    // where the dock will land on release, riding the border nearest the pointer (two strips when bent)
    Window {
        id: ghost
        flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput
                | Qt.WindowDoesNotAcceptFocus
        color: "transparent"
        visible: dock.placing
        x: dock.area.x
        y: dock.area.y
        width: dock.area.width
        height: dock.area.height

        Repeater {
            model: dock.placeBent ? 2 : 1
            Rectangle {
                required property int index
                readonly property rect r: dock.rectOf(index, dock.placeBent, dock.placeSplit,
                                                      index === 0 ? dock.placeEdge : dock.placeEdge2,
                                                      index === 0 ? dock.placeEdge2 : dock.placeEdge,
                                                      dock.placeFrac, dock.thickness)
                x: r.x - dock.area.x + dock.margin
                y: r.y - dock.area.y + dock.margin
                width: r.width - 2 * dock.margin
                height: r.height - 2 * dock.margin
                radius: 18
                color: Qt.alpha(dock.th.accent, 0.22)
                border.width: 2
                border.color: dock.th.accent
            }
        }
    }

    function showTip(text, item) {
        tipAnchor = item.mapToGlobal(item.width / 2, item.height / 2)
        tipEdge = item.Window.window === dock2 ? edge2 : edge
        tip.label = text
    }
    property point tipAnchor
    property string tipEdge: edge

    // name label beside the hovered icon; its own click-through window, since the dock window is too slim to hold it
    Window {
        id: tip
        property string label
        flags: Qt.ToolTip | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput
                | Qt.WindowDoesNotAcceptFocus
        color: "transparent"
        visible: label !== "" && dock.visible && dock.openness > 0.9
        width: txt.implicitWidth + 24
        height: 30
        readonly property real gap: dock.cell / 2 + dock.margin + 8
        x: dock.tipEdge === "right" ? dock.tipAnchor.x - gap - width
           : dock.tipEdge === "left" ? dock.tipAnchor.x + gap
           : dock.tipAnchor.x - width / 2
        y: dock.tipEdge === "top" ? dock.tipAnchor.y + gap - height / 2 + 4
           : dock.tipEdge === "bottom" ? dock.tipAnchor.y - gap - height / 2 - 4
           : dock.tipAnchor.y - height / 2

        Rectangle {
            anchors.fill: parent
            radius: 8
            color: dock.th.surface
            border.width: 0
            Text {
                id: txt
                anchors.centerIn: parent
                text: tip.label
                color: dock.th.text
                font.family: "Segoe UI"
                font.pixelSize: 13
            }
        }
    }
}
