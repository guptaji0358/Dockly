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
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: "Collections"
                color: core.theme.text
                font.pixelSize: 26
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            AppButton { primary: true; iconName: "add"; text: "Create Collection"; onClicked: page.host.addCollection() }
        }

        // the collections themselves, as a strip of glowing nodes
        Text {
            text: "YOUR COLLECTIONS"
            color: core.theme.accent
            font.pixelSize: 11
            font.letterSpacing: 4
            font.weight: Font.DemiBold
            Layout.topMargin: 4
        }
        ItemSpace {
            Layout.fillWidth: true
            Layout.preferredHeight: 130
            strip: true
            host: page.host
            model: collections
            selectedId: page.current
            emptyText: "No collections yet.  Create one to launch several items at once."
            onItemClicked: id => page.current = id
        }

        // the selected collection: its members orbit it
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 8
            Text {
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: core.theme.text
                font.pixelSize: 17
                font.weight: Font.DemiBold
                text: page.info.name === undefined ? "Select or create a collection"
                                                   : page.info.name + "  ·  " + members.count + " item(s)"
            }
            AppButton {
                primary: true
                iconName: "open"
                text: "Launch All"
                enabled: page.info.name !== undefined && members.count > 0
                onClicked: core.launchAll(page.current)
            }
            AppButton {
                iconName: "add"
                text: "Add Cards"
                enabled: page.info.name !== undefined
                onClicked: page.host.pickExisting(page.current)
            }
            OrbButton {
                diameter: 38
                iconName: "edit"
                tip: "Rename"
                enabled: page.info.name !== undefined
                onClicked: page.host.edit(page.current)
            }
            OrbButton {
                diameter: 38
                danger: true
                iconName: "trash"
                tip: "Delete collection"
                enabled: page.info.name !== undefined
                onClicked: page.host.remove(page.current)
            }
        }

        ItemSpace {
            Layout.fillWidth: true
            Layout.fillHeight: true
            host: page.host
            model: members
            collectionId: page.current
            coreLabel: page.info.name === undefined ? "" : page.info.name.toUpperCase()
            emptyText: page.info.name === undefined ? ""
                       : "This collection is empty.
Use \"Add Cards\" to fill it."
        }
    }
}
