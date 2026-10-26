import QtQuick
import QtQuick.Controls
import QtQuick.Window
import LH

// The slim launcher strip: icons only, docked to one screen edge, no title bar.
// Collapsed it is a thin accent line on the edge; the pointer reaching the edge slides it open,
// and launching something (or leaving it alone) slides it away again.
Window {
    id: dock

    readonly property var th: core.theme
    readonly property string edge: core.settings.values.dockEdge
    readonly property bool vertical: edge === "left" || edge === "right"
    readonly property bool autoHide: core.settings.values.dockAutoHide && !dockPinned
    readonly property rect area: workArea

    readonly property int cell: 52          // one icon slot
    readonly property int iconSize: 40
    readonly property int thickness: 64     // across the strip
    readonly property int handle: 6         // what stays on the edge while collapsed
    readonly property int margin: 6         // gap between the strip and the screen edge

    property bool shown: true
    property real openness: shown ? 1 : 0
    Behavior on openness { NumberAnimation { duration: core.settings.values.animations ? 160 : 0; easing.type: Easing.OutCubic } }

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

    // Three views, always reached the same way: 0 = favourites (home, or everything until there are any),
    // 1 = the category picker, 2 = the items of one category (or the members of one collection)
    property int view: 0
    property int browseCat: 0
    property bool inCollection: false
    property string collId

    readonly property var cats: [
        {name: "Apps", cat: 0, icon: "software", col: "#4C8DFF"},
        {name: "Documents", cat: 1, icon: "documents", col: "#FFA94D"},
        {name: "Entertainment", cat: 2, icon: "entertainment", col: "#F06595"},
        {name: "Web", cat: 3, icon: "web", col: "#3DDC97"},
        {name: "Collections", cat: 4, icon: "collections", col: "#A78BFA"}
    ]

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
    onOpennessChanged: if (openness === 0) { view = 0; inCollection = false }

    readonly property int slots: (view === 1 ? cats.length : itemModel.count) + 2 // + the header and the gear
    readonly property int length: {
        const wanted = slots * cell + 16
        const room = (vertical ? area.height : area.width) - 80
        return Math.max(cell + 16, Math.min(wanted, room))
    }
    readonly property real current: handle + (thickness - handle) * openness

    visible: (core.settings.values.dockEnabled && dockAllowed) || dockPinned
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    title: "Dockly dock"

    width: vertical ? current : length
    height: vertical ? length : current
    x: edge === "right" ? area.x + area.width - width
       : edge === "left" ? area.x
       : area.x + (area.width - width) / 2
    y: edge === "bottom" ? area.y + area.height - height
       : edge === "top" ? area.y
       : area.y + (area.height - height) / 2

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
        interval: 700
        onTriggered: {
            if (dock.autoHide && !hover.hovered && !dock.menuOpen && !drops.containsDrag)
                dock.shown = false
            interval = 700
        }
    }

    Item {
        id: root
        anchors.fill: parent
        clip: true

        HoverHandler {
            id: hover
            onHoveredChanged: {
                if (hovered) {
                    dock.shown = true
                    hideTimer.stop()
                } else {
                    tip.label = ""
                    hideTimer.interval = 700
                    hideTimer.restart()
                }
            }
        }

        // drop programs, files, folders or links here to add them (and pin them to the dock)
        DropArea {
            id: drops
            anchors.fill: parent
            onEntered: drag => {
                drag.accepted = drag.hasUrls || drag.hasText
                dock.shown = true
                hideTimer.stop()
            }
            onExited: { hideTimer.interval = 700; hideTimer.restart() }
            onDropped: drop => {
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

        // collapsed: a thin line so you can see (and hit) the edge
        Rectangle {
            anchors.fill: parent
            radius: 3
            color: dock.th.accent
            opacity: 0.55 * (1 - dock.openness)
            visible: opacity > 0.01
        }

        // the strip itself, pinned to the screen edge so it slides in and out with the window
        Rectangle {
            id: strip
            width: dock.vertical ? dock.thickness - dock.margin : parent.width
            height: dock.vertical ? parent.height : dock.thickness - dock.margin
            x: dock.edge === "right" ? parent.width - width - dock.margin : (dock.edge === "left" ? dock.margin : 0)
            y: dock.edge === "bottom" ? parent.height - height - dock.margin : (dock.edge === "top" ? dock.margin : 0)
            radius: 18
            color: Qt.alpha(dock.th.surface, core.settings.values.dockOpacity / 100)
            border.width: 0
            opacity: dock.openness

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: dockMenu.popup()
            }

            // favourites / one category's items
            ListView {
                id: list
                anchors.fill: parent
                anchors.margins: 6
                visible: dock.view !== 1
                orientation: dock.vertical ? ListView.Vertical : ListView.Horizontal
                interactive: contentWidth > width || contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: dock.itemModel
                header: Head {}
                footer: Gear {}

                delegate: Slot {
                    id: slot
                    required property string itemId
                    required property string name
                    required property string iconSource
                    required property int category
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
                }
            }

            // the category picker: five big colour-coded icons
            ListView {
                id: catList
                anchors.fill: parent
                anchors.margins: 6
                visible: dock.view === 1
                orientation: list.orientation
                interactive: contentWidth > width || contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                clip: true
                model: dock.cats
                header: Head {}
                footer: Gear {}

                delegate: Slot {
                    id: catSlot
                    required property var modelData
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
                        Icon {
                            anchors.centerIn: parent
                            name: catSlot.modelData.icon
                            size: 24
                            color: catSlot.modelData.col
                        }
                    }
                }
            }
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
        onVisibleChanged: if (!visible) { hideTimer.interval = 700; hideTimer.restart() }
    }

    DockMenu {
        id: tileMenu
        property string itemId
        property bool pinned
        MenuItem { text: "Open"; onTriggered: core.launch(tileMenu.itemId) }
        MenuItem { text: "Edit..."; onTriggered: tray.editInMain(tileMenu.itemId) }
        MenuItem {
            text: "Remove from dock"
            enabled: tileMenu.pinned
            onTriggered: core.setFavorite(tileMenu.itemId, false)
        }
    }

    DockMenu {
        id: dockMenu
        Menu {
            title: "Position"
            Repeater {
                model: [{t: "Right", v: "right"}, {t: "Left", v: "left"}, {t: "Top", v: "top"}, {t: "Bottom", v: "bottom"}]
                MenuItem {
                    required property var modelData
                    text: modelData.t
                    checkable: true
                    checked: dock.edge === modelData.v
                    onTriggered: core.settings.set("dockEdge", modelData.v)
                }
            }
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

    function showTip(text, item) {
        tipAnchor = item.mapToGlobal(item.width / 2, item.height / 2)
        tip.label = text
    }
    property point tipAnchor

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
        x: dock.edge === "right" ? dock.tipAnchor.x - gap - width
           : dock.edge === "left" ? dock.tipAnchor.x + gap
           : dock.tipAnchor.x - width / 2
        y: dock.edge === "bottom" ? dock.tipAnchor.y - gap - height / 2 - 4
           : dock.edge === "top" ? dock.tipAnchor.y + gap - height / 2 + 4
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
