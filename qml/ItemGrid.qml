import QtQuick
import QtQuick.Controls

// Scrollable grid / list of Cards bound to an ItemModel, with an empty-state message,
// entrance / reorder animations and drag & drop.
Item {
    id: root
    property alias model: view.model
    property string emptyText: ""
    property string styleOverride: ""
    property string selectedId: ""
    property string collectionId: ""   // set when this grid lists a collection's members
    property var host

    readonly property string viewStyle: styleOverride !== "" ? styleOverride : core.settings.values.viewStyle
    readonly property string cardSize: core.settings.values.cardSize
    readonly property int cardW: cardSize === "small" ? 128 : (cardSize === "large" ? 210 : 168)
    readonly property bool animated: core.settings.values.animations
    signal itemClicked(string itemId)

    GridView {
        id: view
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        cellWidth: root.viewStyle === "grid" ? root.cardW + 14 : Math.max(240, width - 14)
        cellHeight: root.viewStyle === "grid" ? Math.round(root.cardW * 0.5) + 118
                                              : (root.viewStyle === "compact" ? 44 : 72)
        ScrollBar.vertical: ScrollBar {}

        // cards fade + slide in, staggered
        populate: Transition {
            id: pop
            enabled: root.animated
            SequentialAnimation {
                PropertyAction { property: "opacity"; value: 0 }
                PauseAnimation { duration: Math.min(pop.ViewTransition.index, 20) * 35 }
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; to: 1; duration: 280; easing.type: Easing.OutCubic }
                    NumberAnimation { property: "y"; from: pop.ViewTransition.destination.y + 18
                                      to: pop.ViewTransition.destination.y; duration: 280; easing.type: Easing.OutCubic }
                }
            }
        }
        add: Transition {
            enabled: root.animated
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 240; easing.type: Easing.OutCubic }
                NumberAnimation { property: "scale"; from: 0.85; to: 1; duration: 240; easing.type: Easing.OutBack }
            }
        }
        displaced: Transition {
            enabled: root.animated
            NumberAnimation { properties: "x,y"; duration: 220; easing.type: Easing.OutCubic }
        }

        delegate: Card {
            width: view.cellWidth
            height: view.cellHeight
            cardW: root.cardW
            viewStyle: root.viewStyle
            collectionId: root.collectionId
            selected: root.selectedId === itemId || core.selectedId === itemId
            onOpened: root.host.launch(itemId)
            onMenuRequested: root.host.showMenu(itemId, root.collectionId)
            onClicked: {
                core.select(itemId)
                root.itemClicked(itemId)
            }
            onEditRequested: root.host.edit(itemId)
            onFavoriteRequested: core.setFavorite(itemId, !favorite)
            onDeleteRequested: {
                if (root.collectionId !== "")
                    core.removeFromCollection(root.collectionId, itemId)
                else
                    root.host.remove(itemId)
            }
            onItemDropped: draggedId => {
                if (root.collectionId !== "")
                    core.moveMember(root.collectionId, draggedId, index)
                else
                    core.addToCollection(itemId, [draggedId])
            }
        }
    }

    Text {
        anchors.centerIn: parent
        width: parent.width - 48
        visible: view.count === 0 && root.emptyText !== ""
        text: root.emptyText
        color: core.theme.subtext
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }
}
