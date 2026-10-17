import QtQuick
import QtQuick.Layouts
import LH

// Custom hub: collections on the left, the selected collection's cards on the right.
Item {
    id: page
    property var host
    property string current: ""
    property var info: ({})

    ItemModel { id: collections; category: LH.Collection }
    ItemModel { id: members; mode: LH.ModeMembers; collectionId: page.current }

    function refresh() {
        let i = page.current === "" ? ({}) : core.itemInfo(page.current)
        if (i.name === undefined) {
            // selection vanished (deleted / restored): fall back to the first collection
            page.current = collections.count > 0 ? collections.idAt(0) : ""
            i = page.current === "" ? ({}) : core.itemInfo(page.current)
        }
        page.info = i
    }

    Component.onCompleted: refresh()
    onCurrentChanged: refresh()
    Connections {
        target: core
        function onItemsChanged() { Qt.callLater(page.refresh) }
    }
    Connections {
        target: page.host
        function onCollectionCreated(id) { page.current = id }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Collections"
                color: core.theme.text
                font.pixelSize: 26
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            AppButton { primary: true; text: "Create Collection"; onClicked: page.host.addCollection() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            ItemGrid {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                host: page.host
                model: collections
                styleOverride: "list"
                selectedId: page.current
                emptyText: "No collections yet.\nCreate one to launch several items at once."
                onItemClicked: id => page.current = id
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: core.theme.text
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                        text: page.info.name === undefined ? "Select or create a collection"
                                                           : page.info.name + "  •  " + members.count + " item(s)"
                    }
                    AppButton {
                        primary: true
                        text: "Launch All"
                        enabled: page.info.name !== undefined && members.count > 0
                        onClicked: core.launchAll(page.current)
                    }
                    AppButton {
                        text: "Add Existing Cards"
                        enabled: page.info.name !== undefined
                        onClicked: page.host.pickExisting(page.current)
                    }
                    AppButton {
                        text: "Edit"
                        enabled: page.info.name !== undefined
                        onClicked: page.host.edit(page.current)
                    }
                    AppButton {
                        text: "Delete"
                        danger: true
                        enabled: page.info.name !== undefined
                        onClicked: page.host.remove(page.current)
                    }
                }

                ItemGrid {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    host: page.host
                    model: members
                    collectionId: page.current
                    emptyText: page.info.name === undefined ? ""
                               : "This collection is empty.\nUse \"Add Existing Cards\" to fill it."
                }
            }
        }
    }
}
