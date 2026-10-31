import QtQuick
import QtQuick.Controls
import LH

// One item as a card (grid), row (list) or slim row (compact), drawn in the active skin with
// hover buttons, a hover lift, drag source and drop target.
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

    property string viewStyle: "grid"
    property bool selected: false
    property int cardW: 168
    property string collectionId: ""   // collection this card is listed in ("" = not a member view)
    signal opened()
    signal menuRequested()
    signal clicked()
    signal editRequested()
    signal favoriteRequested()
    signal deleteRequested()
    signal itemDropped(string draggedId)   // another card was dropped on this one

    readonly property var th: core.theme
    readonly property bool animated: core.settings.values.animations
    readonly property bool grid: viewStyle === "grid"
    readonly property bool compact: viewStyle === "compact"
    readonly property int iconSize: grid ? Math.round(cardW * 0.5) : (compact ? 22 : 38)
    readonly property bool hovered: hover.hovered
    readonly property color accent: th.accentOf(category)

    // 0..1 hover progress drives the lift, the glow and the colour cross-fade
    property real t: hovered ? 1 : 0
    Behavior on t { enabled: root.animated; NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    readonly property real lit: Math.max(t, selected ? 1 : 0)
    readonly property bool dropHover: dropArea.containsDrag || fileDrop.containsDrag

    // idle "floating": grid cards drift up and down; hover settles them back into place
    readonly property bool floating: animated && grid && core.settings.values.floatingTiles
    property real floatY: 0
    SequentialAnimation on floatY {
        running: root.floating && root.visible
        loops: Animation.Infinite
        NumberAnimation { from: -3.5; to: 3.5; duration: 1500 + (root.index % 7) * 110; easing.type: Easing.InOutSine }
        NumberAnimation { from: 3.5; to: -3.5; duration: 1500 + (root.index % 7) * 110; easing.type: Easing.InOutSine }
    }
    readonly property real restShadow: floating ? 0.45 : 0

    z: hovered ? 2 : 0
    scale: grid ? 1 + 0.025 * t : 1
    transform: Translate { y: root.grid ? -4 * root.t + (root.floating ? root.floatY * (1 - root.t) : 0) : 0; x: root.grid ? 0 : 3 * root.t }

    HoverHandler { id: hover }

    // editorial: hard offset shadow
    Rectangle {
        visible: root.th.hardShadow
        x: bg.x + 4 + root.t
        y: bg.y + 4 + root.t
        width: bg.width
        height: bg.height
        color: root.th.border
    }
    // soft shadow that grows with the lift
    Rectangle {
        visible: !root.th.hardShadow && Math.max(root.t, root.restShadow) > 0.02
        x: bg.x
        y: bg.y + 6 * Math.max(root.t, root.restShadow)
        width: bg.width
        height: bg.height
        radius: bg.radius
        color: "#000000"
        opacity: 0.22 * Math.max(root.t, root.restShadow)
    }
    // neon: soft halo around the lit card
    Repeater {
        model: root.th.glow ? 3 : 0
        Rectangle {
            required property int index
            anchors.fill: bg
            anchors.margins: -(index + 1) * 2
            radius: bg.radius + (index + 1) * 2
            color: "transparent"
            border.width: 2
            border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, (0.30 - index * 0.09) * root.lit)
        }
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        anchors.margins: 5
        radius: root.th.radius
        color: Qt.tint(root.th.cardColor(root.category, false),
                       Qt.rgba(root.th.cardColor(root.category, true).r, root.th.cardColor(root.category, true).g,
                               root.th.cardColor(root.category, true).b, root.lit))
        border.width: root.dropHover ? 3 : (root.th.borderWidth > 0 ? root.th.borderWidth : (root.lit > 0.4 ? 2 : 0))
        border.color: root.dropHover ? root.th.accent
                      : (root.lit > 0.01 && !root.th.hardShadow
                         ? Qt.tint(root.th.border, Qt.rgba(root.accent.r, root.accent.g, root.accent.b, root.lit))
                         : root.th.border)
    }

    // playful: colourful disc behind the icon
    Rectangle {
        visible: root.th.tinted && root.grid
        width: root.iconSize + 18
        height: width
        radius: width / 2
        x: iconBox.x - 9
        y: iconBox.y - 9
        color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.28)
    }

    Item {
        id: iconBox
        width: root.iconSize
        height: root.iconSize
        x: root.grid ? (root.width - width) / 2 : 20
        y: root.grid ? 22 - 2 * root.t : (root.height - height) / 2
        scale: 1 + 0.05 * root.t
        Image {
            anchors.fill: parent
            source: root.iconSource
            sourceSize: Qt.size(root.iconSize * 2, root.iconSize * 2)
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            opacity: root.status === 0 ? 1 : 0.45
        }
        Icon {
            anchors.centerIn: parent
            visible: root.status !== 0
            name: "warning"
            size: Math.round(root.iconSize * 0.6)
            color: root.status === 2 ? root.th.missing : root.th.warning
        }
    }

    Text {
        id: nameText
        text: root.name
        color: root.th.text
        font.weight: Font.DemiBold
        font.family: root.th.style === "editorial" ? root.th.headingFont : "Segoe UI"
        elide: Text.ElideRight
        horizontalAlignment: root.grid ? Text.AlignHCenter : Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        x: root.grid ? 14 : iconBox.x + iconBox.width + 14
        width: root.grid ? root.width - 28 : Math.max(40, buttons.x - x - 8)
        y: root.grid ? iconBox.y + iconBox.height + 14 : (root.compact ? (root.height - height) / 2 : 13)
    }
    Text {
        visible: !root.compact
        text: root.grid ? root.subtitle
                        : (root.status !== 0 ? root.subtitle + " — " + root.target
                                             : (root.target !== "" ? root.target : root.subtitle))
        color: root.status === 2 ? root.th.missing : (root.status === 1 ? root.th.warning : root.th.subtext)
        font.pixelSize: 12
        elide: root.grid ? Text.ElideRight : Text.ElideMiddle
        horizontalAlignment: root.grid ? Text.AlignHCenter : Text.AlignLeft
        x: nameText.x
        width: nameText.width
        y: root.grid ? nameText.y + nameText.height + 3 : 40
    }

    // whole-card interaction (underneath the buttons)
    MouseArea {
        id: mouse
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton)
                root.menuRequested()
            else
                root.clicked()
        }
        onDoubleClicked: mouse => { if (mouse.button === Qt.LeftButton) root.opened() }
    }

    // hover buttons: edit, favorite, delete (a lone star stays visible for favorites)
    Row {
        id: buttons
        spacing: 4
        x: root.width - width - 14
        y: root.grid ? 14 : (root.height - height) / 2
        Repeater {
            model: [
                {icon: "open", kind: "launch"},
                {icon: "edit", kind: "edit"},
                {icon: root.favorite ? "star-fill" : "star", kind: "fav"},
                {icon: "trash", kind: "del"}
            ]
            delegate: Rectangle {
                id: btn
                required property var modelData
                // "launch" is only for collections: it opens everything inside
                readonly property bool shown: modelData.kind === "launch" ? root.hovered && root.category === LH.Collection
                                                                          : root.hovered || (modelData.kind === "fav" && root.favorite)
                width: 28
                height: 28
                visible: shown
                radius: root.th.radius === 0 ? 0 : 8
                color: root.hovered ? (btnMouse.containsMouse ? root.th.cardHover : root.th.surface) : "transparent"
                border.width: root.hovered && root.th.hardShadow ? 2 : 0
                border.color: root.th.border
                Icon {
                    anchors.centerIn: parent
                    size: 16
                    name: btn.modelData.icon
                    color: btn.modelData.kind === "del" ? root.th.missing
                         : (btn.modelData.kind === "fav" && root.favorite ? root.th.favorite : root.th.text)
                }
                MouseArea {
                    id: btnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (btn.modelData.kind === "launch")
                            root.opened()
                        else if (btn.modelData.kind === "edit")
                            root.editRequested()
                        else if (btn.modelData.kind === "fav")
                            root.favoriteRequested()
                        else
                            root.deleteRequested()
                    }
                }
                ToolTip.visible: btnMouse.containsMouse
                ToolTip.delay: 400
                ToolTip.text: modelData.kind === "launch" ? "Launch All" : (modelData.kind === "edit" ? "Edit" : (modelData.kind === "fav" ? "Favorite" : "Delete"))
            }
        }
    }

    // ---- drag & drop ---------------------------------------------------------
    // Drag this card (onto a collection, or within a collection to reorder).
    Drag.dragType: Drag.Automatic
    Drag.supportedActions: Qt.CopyAction
    Drag.keys: ["application/x-dockly-item"]
    Drag.mimeData: ({
        "application/x-dockly-item": root.itemId,
        "application/x-dockly-source": root.collectionId
    })
    DragHandler {
        id: dragHandler
        target: null
        acceptedButtons: Qt.LeftButton
        onActiveChanged: {
            if (active) {
                root.grabToImage(r => {
                    root.Drag.imageSource = r.url
                    root.Drag.hotSpot = Qt.point(root.width / 2, root.height / 2)
                    root.Drag.active = dragHandler.active
                })
            } else {
                root.Drag.active = false
            }
        }
    }
    // Drop target: collection cards accept cards; member cards accept siblings for reordering.
    DropArea {
        id: dropArea
        anchors.fill: parent
        keys: ["application/x-dockly-item"]
        enabled: root.category === LH.Collection || root.collectionId !== ""
        onDropped: drop => {
            const id = drop.getDataAsString("application/x-dockly-item")
            const src = drop.getDataAsString("application/x-dockly-source")
            if (id === "" || id === root.itemId)
                return
            if (root.category === LH.Collection && root.collectionId === "") {
                root.itemDropped(id)
            } else if (root.collectionId !== "" && src === root.collectionId) {
                root.itemDropped(id)
            }
        }
    }

    // Files, folders or links from Explorer / the browser dropped on a collection go into it (and into the library).
    DropArea {
        id: fileDrop
        anchors.fill: parent
        enabled: root.category === LH.Collection && root.collectionId === ""
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
            drop.accept()
            core.addDroppedTo(root.itemId, items)
        }
    }

    ToolTip.visible: mouse.containsMouse && !root.hovered
    ToolTip.delay: 900
    ToolTip.text: root.target !== "" ? root.target : root.name
}
