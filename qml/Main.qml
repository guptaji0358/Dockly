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
    title: "Launcher Hub"
    color: th.bg
    font.family: "Segoe UI"
    font.pixelSize: 14

    readonly property var th: core.theme
    property int tab: 0
    property var ctx: ({})
    property string ctxCollection: ""
    property var ctxCollections: []
    property string pendingAddItem: ""
    signal collectionCreated(string id)

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
        if (files.length > 0)
            core.addFiles(category, files)
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
        if (core.settings.values.animations)
            fade.restart()
    }
    Component.onCompleted: {
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

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Icon-only rail (hidden when the floating dock is selected)
        Rectangle {
            visible: !win.th.dock
            Layout.preferredWidth: 76
            Layout.fillHeight: true
            color: win.th.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                anchors.topMargin: 16
                spacing: 6

                Icon {
                    Layout.alignment: Qt.AlignHCenter
                    name: "logo"
                    size: 38
                    color: "white"
                }
                Item { Layout.preferredHeight: 12 }

                Repeater {
                    model: win.tabs
                    delegate: Rectangle {
                        id: navItem
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        radius: win.th.radius === 0 ? 0 : 12
                        color: win.tab === index ? win.th.accent : (navMouse.containsMouse ? "#22ffffff" : "transparent")
                        Behavior on color { enabled: core.settings.values.animations; ColorAnimation { duration: 140 } }
                        scale: navMouse.pressed ? 0.94 : 1
                        Behavior on scale { enabled: core.settings.values.animations; NumberAnimation { duration: 90 } }
                        Icon {
                            anchors.centerIn: parent
                            name: navItem.modelData.icon
                            size: 24
                            color: win.tab === navItem.index ? "white" : win.th.sidebarText
                        }
                        MouseArea {
                            id: navMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: win.tab = navItem.index
                        }
                        ToolTip.visible: navMouse.containsMouse
                        ToolTip.delay: 400
                        ToolTip.text: navItem.modelData.full + "   (Ctrl+" + (navItem.index + 1) + ")"
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

        // Main column
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 22
            Layout.rightMargin: 14
            Layout.bottomMargin: 8
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                Row {
                    visible: win.th.dock
                    spacing: 8
                    Icon { name: "logo"; size: 28; color: win.th.accent; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: "Launcher Hub"
                        color: win.th.text
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        font.family: win.th.headingFont
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                TextField {
                    id: search
                    Layout.fillWidth: true
                    implicitHeight: 40
                    placeholderText: "Search everything...   (Ctrl+F)"
                    color: win.th.text
                    placeholderTextColor: win.th.subtext
                    selectByMouse: true
                    leftPadding: 44
                    background: Rectangle {
                        radius: win.th.radius === 0 ? 0 : 10
                        color: win.th.surface
                        border.width: win.th.style === "editorial" ? 2 : 1
                        border.color: search.activeFocus ? win.th.accent : win.th.border
                        Icon { x: 15; anchors.verticalCenter: parent.verticalCenter; name: "search"; size: 18; color: win.th.subtext }
                    }
                    onTextChanged: {
                        // Global search: any text jumps to Home and shows matches across every hub.
                        if (text.trim() !== "" && win.tab !== 0)
                            win.tab = 0
                    }
                    Shortcut { sequences: [StandardKey.Find]; onActivated: { search.forceActiveFocus(); search.selectAll() } }
                }
                ViewToolbar {}
            }

            StackLayout {
                id: stack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: win.tab

                HomePage { host: win; search: search.text }
                CategoryPage { host: win; category: LH.Software }
                CategoryPage { host: win; category: LH.Document }
                CategoryPage { host: win; category: LH.Entertainment }
                CategoryPage { host: win; category: LH.Web }
                CollectionsPage { host: win }
                SettingsPage { host: win }
            }

            // Floating dock navigation
            Rectangle {
                id: dock
                visible: win.th.dock
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: dockRow.implicitWidth + 20
                implicitHeight: 54
                radius: win.th.radius === 0 ? 0 : 27
                color: win.th.sidebar
                border.width: Math.max(1, win.th.borderWidth)
                border.color: win.th.border
                Row {
                    id: dockRow
                    anchors.centerIn: parent
                    spacing: 4
                    Repeater {
                        model: win.tabs
                        delegate: Rectangle {
                            id: dockItem
                            required property int index
                            required property var modelData
                            width: 52
                            height: 40
                            radius: win.th.radius === 0 ? 0 : 20
                            color: win.tab === index ? win.th.accent : (dockMouse.containsMouse ? "#22ffffff" : "transparent")
                            Behavior on color { enabled: core.settings.values.animations; ColorAnimation { duration: 140 } }
                            scale: dockMouse.containsMouse ? 1.08 : 1
                            Behavior on scale { enabled: core.settings.values.animations; NumberAnimation { duration: 120 } }
                            Icon {
                                anchors.centerIn: parent
                                name: dockItem.modelData.icon
                                size: 24
                                color: win.tab === dockItem.index ? "white" : win.th.sidebarText
                            }
                            MouseArea {
                                id: dockMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: win.tab = dockItem.index
                            }
                            ToolTip.visible: dockMouse.containsMouse
                            ToolTip.delay: 400
                            ToolTip.text: dockItem.modelData.full
                        }
                    }
                }
            }

            Text {
                id: toast
                Layout.fillWidth: true
                color: win.th.subtext
                elide: Text.ElideRight
                Timer { id: toastTimer; interval: 5000; onTriggered: toast.text = "" }
            }
        }

        // Detail panel for the selected card (slides in from the right)
        Item {
            id: panelHost
            Layout.preferredWidth: panel.shown ? 318 : 0
            Layout.fillHeight: true
            clip: true
            visible: width > 1
            Behavior on Layout.preferredWidth {
                enabled: core.settings.values.animations
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
            DetailPanel {
                id: panel
                host: win
                x: 0
                y: 22
                width: 300
                height: parent.height - 44
                opacity: shown ? 1 : 0
                Behavior on opacity { enabled: core.settings.values.animations; NumberAnimation { duration: 200 } }
            }
        }
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
            const hints = [-1, LH.Software, LH.Document, LH.Entertainment, LH.Web, -1, -1]
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
        model: 7
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
    PickSoftwareDialog { id: softwareDialog }
    ChoiceDialog { id: choice }
}
