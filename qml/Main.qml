import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import LH

ApplicationWindow {
    id: win
    width: 1240
    height: 780
    minimumWidth: 900
    minimumHeight: 560
    title: "Dockly"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: th.bg
    font.family: "Segoe UI"
    font.pixelSize: 14

    readonly property var th: core.theme
    property int tab: 0
    Binding { target: Fx; property: "windowActive"; value: win.active }
    property var ctx: ({})
    property string ctxCollection: ""
    property var ctxCollections: []
    property string pendingAddItem: ""
    signal collectionCreated(string id)
    ItemModel { id: diagModel; category: LH.Software }   // used by the --dialog panel diagnostic

    palette {
        window: th.bg
        windowText: th.text
        base: th.surface
        text: th.text
        button: th.surface
        buttonText: th.text
        highlight: th.accent
        highlightedText: "white"
        placeholderText: th.subtext
        mid: th.border
        dark: th.border
        light: th.cardHover
    }

    readonly property var tabs: [
        {icon: "home", name: "Home", full: "Home"},
        {icon: "software", name: "Software", full: "Software Hub"},
        {icon: "documents", name: "Documents", full: "Documents Hub"},
        {icon: "entertainment", name: "Entertainment", full: "Entertainment Hub"},
        {icon: "web", name: "Web", full: "Web Shortcuts"},
        {icon: "collections", name: "Collections", full: "Collections"},
        {icon: "github", name: "GitHub", full: "GitHub Repos"},
        {icon: "connector", name: "Connectors", full: "Connectors"},
        {icon: "settings", name: "Settings", full: "Settings"}
    ]

    // ---- actions shared by every page ---------------------------------

    function launch(id) {
        if (core.launch(id) !== LH.TargetMissing)
            return
        // Item exists? -> No: warn and offer the recovery actions.
        const i = core.itemInfo(id)
        const isWeb = i.category === LH.Web
        const labels = isWeb ? ["Edit URL", "Delete", "Cancel"] : ["Locate Again", "Edit Path", "Delete", "Cancel"]
        const text = isWeb ? "The URL looks invalid:\n" + i.target : "Nothing was found at:\n" + i.target
        choice.ask(isWeb ? "Invalid link" : "Item missing", "\"" + i.name + "\" can't be opened.\n\n" + text, labels, k => {
            const a = labels[k]
            if (a === "Locate Again") {
                const f = core.pickFiles("Locate " + i.name, core.fileFilter(i.category), false)
                if (f.length > 0)
                    core.updateItem(id, i.name, f[0])
            } else if (a === "Edit Path" || a === "Edit URL") {
                edit(id)
            } else if (a === "Delete") {
                remove(id)
            }
        }, labels.indexOf("Delete"))
    }

    function edit(id) { itemDialog.openEdit(id) }

    function remove(id) {
        const i = core.itemInfo(id)
        if (i.name === undefined)
            return
        if (!core.settings.values.confirmDelete) {
            core.removeItem(id)
            return
        }
        let note = ""
        if (i.category === LH.Collection)
            note = "\n\nThe items inside the collection are not deleted."
        else if (i.category === LH.Software)
            note = "\n\nOnly the card is removed; the program stays installed and in the catalog."
        choice.ask("Delete card", "Delete \"" + i.name + "\"?" + note, ["Delete", "Cancel"],
                   k => { if (k === 0) core.removeItem(id) }, 0)
    }

    function showMenu(id, collectionId) {
        ctx = core.itemInfo(id)
        ctxCollection = collectionId
        ctxCollections = core.collectionsList()
        ctxMenu.popup()
    }

    function addFiles(category) {
        const files = core.pickFiles(category === LH.Document ? "Add Document" : "Add Media",
                                     core.fileFilter(category), true)
        if (files.length === 1)
            itemDialog.openAddTarget(category, files[0], "") // one file: offer name + icon first
        else if (files.length > 1)
            core.addFiles(category, files) // several: added directly, icons via Edit
    }
    function addSoftware() { softwareDialog.openPicker() }
    function addUrl() { itemDialog.openAdd(LH.Web) }
    function addCollection() { itemDialog.openAdd(LH.Collection) }
    function pickExisting(collectionId) { pickDialog.openFor(collectionId) }
    function ask(title, text, labels, callback, dangerIndex) { choice.ask(title, text, labels, callback, dangerIndex) }

    function goTo(i) { tab = i }

    // ---- window behaviour -----------------------------------------------

    onClosing: close => {
        if (tray.active) {
            close.accepted = false
            hide()
        } else {
            Qt.quit()
        }
    }
    onVisibilityChanged: v => {
        if (v === Window.Minimized && tray.active)
            Qt.callLater(hide)
    }
    onTabChanged: {
        if (tabs[tab].name === "GitHub")
            Qt.callLater(offerOrganize)
        if (core.settings.values.animations)
            fade.restart()
    }
    // GitHub repos that were added before groups existed: offer to tidy them (asked once per launch)
    property bool askedOrganize: false
    function offerOrganize() {
        if (askedOrganize)
            return
        const n = core.connectors.looseGithubCount()
        if (n === 0)
            return
        askedOrganize = true
        ask("Organize your GitHub repos?",
            n + " GitHub repo(s) are mixed in with your Web shortcuts.\n\n"
            + "Organize automatically files them into one category per repo owner, and you can still move them later. "
            + "Nothing is deleted.",
            ["Organize automatically", "Not now"], k => { if (k === 0) core.connectors.organizeGithub() })
    }

    Component.onCompleted: {
        if (startRing)
            ring.opened = true
        if (startDialog === "panel")
            core.select(diagModel.idAt(0))
        if (startDialog === "software") {
            core.refreshSoftware()
            softwareDialog.openPicker()
        }
        if (startPage >= 0) {
            tab = startPage
        } else {
            const d = core.settings.values.defaultTab
            tab = d === "software" ? 1 : (d === "custom" ? 5 : 0)
        }
    }

    Connections {
        target: core
        function onMessage(text) {
            toast.text = text
            toastTimer.restart()
        }
    }

    // ---- layout -----------------------------------------------------------

    Nebula { id: nebula; anchors.fill: parent }

    // pointer position feeds the star-field parallax
    HoverHandler {
        enabled: Fx.parallax
        onPointChanged: {
            nebula.px = point.position.x / Math.max(1, win.width)
            nebula.py = point.position.y / Math.max(1, win.height)
        }
    }

    // ---- custom title bar (replaces the native Windows one) ------------------
    Item {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 34
        z: 40

        readonly property bool maxed: win.visibility === Window.Maximized

        // drag to move, double-click to maximize / restore
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            // start the OS move on first drag, not on press, so double-click still arrives
            onPositionChanged: if (pressed) win.startSystemMove()
            onDoubleClicked: titleBar.maxed ? win.showNormal() : win.showMaximized()
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Icon { name: "logo"; size: 16; color: win.th.accent; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: win.title
                color: win.th.subtext
                font.pixelSize: 12
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Row {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            Repeater {
                model: [
                    {glyph: "", act: 0},
                    {glyph: "", act: 1},
                    {glyph: "", act: 2}
                ]
                delegate: Rectangle {
                    id: cap
                    required property var modelData
                    width: 46
                    height: titleBar.height
                    color: capMouse.containsMouse
                           ? (modelData.act === 2 ? "#e81123" : Qt.rgba(1, 1, 1, 0.12))
                           : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: cap.modelData.act === 1 && titleBar.maxed ? "" : cap.modelData.glyph
                        font.family: "Segoe MDL2 Assets"
                        font.pixelSize: 10
                        color: capMouse.containsMouse && cap.modelData.act === 2 ? "white" : win.th.text
                    }
                    MouseArea {
                        id: capMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (cap.modelData.act === 0)
                                win.showMinimized()
                            else if (cap.modelData.act === 1)
                                titleBar.maxed ? win.showNormal() : win.showMaximized()
                            else
                                win.close()
                        }
                    }
                }
            }
        }
    }

    // ---- edge / corner resize handles (frameless windows have none) ------------
    Repeater {
        model: [
            {e: Qt.LeftEdge, cur: Qt.SizeHorCursor},
            {e: Qt.RightEdge, cur: Qt.SizeHorCursor},
            {e: Qt.TopEdge, cur: Qt.SizeVerCursor},
            {e: Qt.BottomEdge, cur: Qt.SizeVerCursor},
            {e: Qt.LeftEdge | Qt.TopEdge, cur: Qt.SizeFDiagCursor},
            {e: Qt.RightEdge | Qt.BottomEdge, cur: Qt.SizeFDiagCursor},
            {e: Qt.RightEdge | Qt.TopEdge, cur: Qt.SizeBDiagCursor},
            {e: Qt.LeftEdge | Qt.BottomEdge, cur: Qt.SizeBDiagCursor}
        ]
        delegate: MouseArea {
            required property var modelData
            readonly property bool l: (modelData.e & Qt.LeftEdge) !== 0
            readonly property bool r: (modelData.e & Qt.RightEdge) !== 0
            readonly property bool t: (modelData.e & Qt.TopEdge) !== 0
            readonly property bool b: (modelData.e & Qt.BottomEdge) !== 0
            readonly property bool corner: (l || r) && (t || b)
            enabled: !titleBar.maxed
            z: 50
            x: l ? 0 : (r ? win.width - width : 8)
            y: t ? 0 : (b ? win.height - height : 8)
            width: (l || r) ? (corner ? 10 : 5) : win.width - 16
            height: (t || b) ? (corner ? 10 : 5) : win.height - 16
            cursorShape: modelData.cur
            onPressed: win.startSystemResize(modelData.e)
        }
    }

    // ---- HUD: top bar ---------------------------------------------------------
    Item {
        id: top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: titleBar.bottom
        anchors.margins: 20
        anchors.topMargin: 8
        height: 46

        Row {
            id: brand
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            Icon { name: "logo"; size: 28; color: win.th.accent; anchors.verticalCenter: parent.verticalCenter }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    text: "DOCKLY"
                    color: win.th.text
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    font.letterSpacing: 5
                }
                Text {
                    text: win.tabs[win.tab].full.toUpperCase()
                    color: win.th.accent
                    font.pixelSize: 10
                    font.letterSpacing: 3
                }
            }
        }

        Glass {
            id: searchBox
            anchors.centerIn: parent
            width: Math.max(180, Math.min(560, parent.width - brand.width - tools.width - 70))
            height: 42
            radius: 21
            border.color: search.activeFocus ? win.th.accent : edge
            tint: 0.5
            Icon { x: 16; anchors.verticalCenter: parent.verticalCenter; name: "search"; size: 18; color: win.th.subtext }
            TextField {
                id: search
                anchors.fill: parent
                leftPadding: 46
                rightPadding: 16
                placeholderText: "Search everything…   ( / )"
                color: win.th.text
                placeholderTextColor: win.th.subtext
                selectByMouse: true
                verticalAlignment: TextInput.AlignVCenter
                background: null
                onTextChanged: {
                    // any text jumps Home and searches every hub
                    if (text.trim() !== "" && win.tab !== 0)
                        win.tab = 0
                }
                Keys.onEscapePressed: { text = ""; focus = false }
            }
            Shortcut { sequences: [StandardKey.Find, "/"]; onActivated: { search.forceActiveFocus(); search.selectAll() } }
        }

        Row {
            id: tools
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            ViewToolbar { visible: core.settings.values.layoutMode === "grid"; anchors.verticalCenter: parent.verticalCenter }

            Glass {
                id: layoutSwitch
                height: 42
                width: 3 * 42 + 8
                radius: 21
                tint: 0.5
                Row {
                    anchors.centerIn: parent
                    spacing: 0
                    Repeater {
                        model: [{k: "hex", tip: "Honeycomb"}, {k: "orbit", tip: "Orbit"}, {k: "grid", tip: "Classic grid"}]
                        delegate: Item {
                            id: seg
                            required property var modelData
                            readonly property bool on: core.settings.values.layoutMode === modelData.k
                            width: 42; height: 42
                            Rectangle {
                                anchors.centerIn: parent
                                width: 34; height: 34; radius: 17
                                color: seg.on ? win.th.accent : (segMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.10) : "transparent")
                                Behavior on color { enabled: Fx.animated; ColorAnimation { duration: 120 } }
                            }
                            Icon { anchors.centerIn: parent; name: seg.modelData.k; size: 20; color: seg.on ? "white" : win.th.text }
                            MouseArea {
                                id: segMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: core.settings.set("layoutMode", seg.modelData.k)
                            }
                            ToolTip.visible: segMouse.containsMouse
                            ToolTip.delay: 400
                            ToolTip.text: seg.modelData.tip
                        }
                    }
                }
            }

            Glass {
                height: 42
                width: clock.implicitWidth + 28
                radius: 21
                tint: 0.5
                Text {
                    id: clock
                    anchors.centerIn: parent
                    color: win.th.text
                    font.pixelSize: 14
                    font.letterSpacing: 2
                    text: Qt.formatTime(new Date(), "hh:mm")
                    Timer {
                        interval: 15000; running: Fx.live; repeat: true
                        onTriggered: clock.text = Qt.formatTime(new Date(), "hh:mm")
                    }
                }
            }
        }
    }

    // ---- pages ---------------------------------------------------------------------
    StackLayout {
        id: stack
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: top.bottom
        anchors.bottom: parent.bottom
        anchors.leftMargin: 24
        anchors.rightMargin: panel.shown ? 352 : 24   // pages make room for the inspector
        Behavior on anchors.rightMargin { enabled: Fx.animated; NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
        anchors.topMargin: 10
        anchors.bottomMargin: 96
        currentIndex: win.tab

        HomePage { host: win; search: search.text }
        CategoryPage { host: win; category: LH.Software }
        CategoryPage { host: win; category: LH.Document }
        CategoryPage { host: win; category: LH.Entertainment }
        CategoryPage { host: win; category: LH.Web }
        CollectionsPage { host: win }
        GitHubPage { host: win }
        ConnectorsPage { host: win }
        SettingsPage { host: win }
    }

    // ---- floating detail panel ---------------------------------------------------------
    Item {
        id: panelHost
        anchors.right: parent.right
        anchors.top: top.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: 14
        anchors.bottomMargin: 96
        anchors.rightMargin: 20
        width: 318
        visible: panel.shown || panel.x < 330
        DetailPanel {
            id: panel
            host: win
            width: 300
            height: parent.height
            x: shown ? 0 : 340
            opacity: shown ? 1 : 0
            Behavior on x { enabled: Fx.animated; NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
            Behavior on opacity { enabled: Fx.animated; NumberAnimation { duration: 200 } }
        }
    }

    // ---- orb: opens the radial ring --------------------------------------------------------
    Item {
        id: orb
        width: 66; height: 66
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 22
        z: 20
        scale: orbMouse.pressed ? 0.92 : (orbMouse.containsMouse ? 1.08 : 1)
        Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 120 } }

        Rectangle {   // pulsing aura
            anchors.centerIn: parent
            width: 66; height: 66; radius: 33
            color: win.th.accent
            opacity: 0.18
            visible: Fx.glow
            SequentialAnimation on scale {
                running: Fx.live && Fx.glow
                loops: Animation.Infinite
                NumberAnimation { to: 1.55; duration: 1800; easing.type: Easing.InOutSine }
                NumberAnimation { to: 1.0; duration: 1800; easing.type: Easing.InOutSine }
            }
        }
        Rectangle {
            anchors.fill: parent
            radius: 33
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.lighter(win.th.accent, 1.35) }
                GradientStop { position: 1; color: Qt.darker(win.th.accent, 1.3) }
            }
            border.width: 2
            border.color: Qt.rgba(1, 1, 1, 0.35)
        }
        Icon { anchors.centerIn: parent; name: "logo"; size: 32; color: "white" }
        MouseArea {
            id: orbMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton
            onClicked: ring.opened = !ring.opened
        }
        ToolTip.visible: orbMouse.containsMouse && !ring.opened
        ToolTip.delay: 500
        ToolTip.text: "Open the ring   (Ctrl+Space)"
    }

    Text {
        id: toast
        anchors.left: parent.left
        anchors.leftMargin: 28
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        width: parent.width / 2 - 120
        color: win.th.subtext
        elide: Text.ElideRight
        Timer { id: toastTimer; interval: 5000; onTriggered: toast.text = "" }
    }

    Ring {
        id: ring
        anchors.fill: parent
        z: 30
        tabs: win.tabs
        current: win.tab
        host: win
        onPicked: i => win.tab = i
    }

    Shortcut { sequences: ["Ctrl+Space", "F2"]; onActivated: ring.opened = !ring.opened }
    // middle click anywhere toggles the ring too
    MouseArea {
        anchors.fill: parent
        z: -2
        acceptedButtons: Qt.MiddleButton
        onClicked: ring.opened = !ring.opened
    }

    // ---- drag & drop of files / folders / links onto the window ---------------------
    DropArea {
        id: windowDrop
        anchors.fill: parent
        z: -1
        onEntered: drag => {
            if (!drag.hasUrls && !(drag.hasText && drag.text.indexOf("http") === 0))
                drag.accepted = false
        }
        onDropped: drop => {
            const items = []
            if (drop.hasUrls) {
                for (let i = 0; i < drop.urls.length; ++i)
                    items.push(drop.urls[i].toString())
            } else if (drop.hasText) {
                items.push(drop.text.trim())
            }
            // the tab you're on hints where files of an unknown type go
            const hints = [-1, LH.Software, LH.Document, LH.Entertainment, LH.Web, -1, -1, -1, -1]
            core.addDropped(items, hints[win.tab])
        }
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 12
        z: 100
        visible: opacity > 0.01
        opacity: windowDrop.containsDrag ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 140 } }
        radius: win.th.radius === 0 ? 0 : 18
        color: Qt.rgba(win.th.accent.r, win.th.accent.g, win.th.accent.b, 0.14)
        border.width: 3
        border.color: win.th.accent
        Text {
            anchors.centerIn: parent
            text: "Drop files, folders or links to add them"
            color: win.th.accent
            font.pixelSize: 24
            font.weight: Font.DemiBold
        }
    }

    NumberAnimation { id: fade; target: stack; property: "opacity"; from: 0; to: 1; duration: 180 }

    Repeater {
        model: 8
        delegate: Item {
            required property int index
            Shortcut { sequence: "Ctrl+" + (index + 1); onActivated: win.tab = index }
        }
    }

    // ---- context menu -----------------------------------------------------

    component Styled: Menu {
        background: Rectangle {
            color: win.th.surface
            border.color: win.th.border
            radius: win.th.radius === 0 ? 0 : 8
        }
    }

    Styled {
        id: ctxMenu
        MenuItem {
            text: win.ctx.category === LH.Collection ? "Launch All" : "Open"
            onTriggered: win.launch(win.ctx.id)
        }
        MenuItem {
            text: win.ctx.favorite ? "Remove from Favorites" : "Add to Favorites"
            onTriggered: core.setFavorite(win.ctx.id, !win.ctx.favorite)
        }
        MenuSeparator {}
        MenuItem {
            text: win.ctx.category === LH.Collection ? "Rename..." : "Edit..."
            onTriggered: win.edit(win.ctx.id)
        }
        MenuItem {
            text: "Change Icon..."
            onTriggered: win.edit(win.ctx.id)
        }
        MenuItem {
            readonly property bool applies: win.ctx.status !== 0 && win.ctx.category !== LH.Web
                                            && win.ctx.category !== LH.Collection
            text: "Locate Again..."
            visible: applies
            height: applies ? implicitHeight : 0
            onTriggered: {
                const f = core.pickFiles("Locate " + win.ctx.name, core.fileFilter(win.ctx.category), false)
                if (f.length > 0)
                    core.updateItem(win.ctx.id, win.ctx.name, f[0])
            }
        }
        MenuItem {
            readonly property bool applies: win.ctx.category !== LH.Web && win.ctx.category !== LH.Collection
            text: "Show in Folder"
            visible: applies
            height: applies ? implicitHeight : 0
            onTriggered: core.showInFolder(win.ctx.id)
        }
        Styled {
            id: colMenu
            title: "Add to Collection"
            enabled: win.ctx.category !== LH.Collection
            Instantiator {
                model: win.ctxCollections
                delegate: MenuItem {
                    required property var modelData
                    text: modelData.name
                    onTriggered: core.addToCollection(modelData.id, [win.ctx.id])
                }
                onObjectAdded: (index, object) => colMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => colMenu.removeItem(object)
            }
            MenuItem {
                text: "New Collection..."
                onTriggered: {
                    win.pendingAddItem = win.ctx.id
                    win.addCollection()
                }
            }
        }
        MenuItem {
            readonly property bool applies: win.ctxCollection !== ""
            text: "Remove from this Collection"
            visible: applies
            height: applies ? implicitHeight : 0
            onTriggered: core.removeFromCollection(win.ctxCollection, win.ctx.id)
        }
        MenuSeparator {}
        MenuItem {
            text: "Delete"
            onTriggered: win.remove(win.ctx.id)
        }
    }

    // ---- dialogs ------------------------------------------------------------

    ItemDialog {
        id: itemDialog
        onCreated: id => {
            if (category === LH.Collection) {
                if (win.pendingAddItem !== "")
                    core.addToCollection(id, [win.pendingAddItem])
                win.collectionCreated(id)
            }
            win.pendingAddItem = ""
        }
        onClosed: win.pendingAddItem = ""
    }
    PickDialog { id: pickDialog }
    PickSoftwareDialog {
        id: softwareDialog
        onCustomize: (target, name) => itemDialog.openAddTarget(LH.Software, target, name)
    }
    ChoiceDialog { id: choice }
}
