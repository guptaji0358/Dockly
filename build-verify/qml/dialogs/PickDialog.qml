import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Multi-select list of existing items to add to a collection.
AppDialog {
    id: dlg
    property string collectionId: ""
    property var entries: []
    property var picked: ({})
    readonly property var visibleEntries: {
        const q = filter.text.trim().toLowerCase()
        return q === "" ? entries : entries.filter(e => e.name.toLowerCase().indexOf(q) >= 0)
    }

    title: "Add Existing Cards"

    function openFor(id) {
        collectionId = id
        entries = core.addableItems(id)
        picked = ({})
        filter.text = ""
        open()
    }

    TextField {
        id: filter
        Layout.fillWidth: true
        implicitHeight: 38
        placeholderText: "Filter..."
        color: dlg.th.text
        placeholderTextColor: dlg.th.subtext
        background: Rectangle {
            radius: 8
            color: dlg.th.bg
            border.color: filter.activeFocus ? dlg.th.accent : dlg.th.border
        }
    }
    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: 320
        clip: true
        model: dlg.visibleEntries
        ScrollBar.vertical: ScrollBar {}
        delegate: CheckDelegate {
            required property var modelData
            width: ListView.view.width
            text: modelData.name + "   (" + modelData.category + ")"
            checked: dlg.picked[modelData.id] === true
            onToggled: dlg.picked[modelData.id] = checked
            contentItem: Text {
                text: parent.text
                color: dlg.th.text
                verticalAlignment: Text.AlignVCenter
                leftPadding: 4
                elide: Text.ElideRight
            }
        }
        Text {
            anchors.centerIn: parent
            visible: parent.count === 0
            text: "Nothing to add."
            color: dlg.th.subtext
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        Item { Layout.fillWidth: true }
        AppButton { text: "Cancel"; onClicked: dlg.close() }
        AppButton {
            text: "Add Selected"
            primary: true
            onClicked: {
                core.addToCollection(dlg.collectionId, Object.keys(dlg.picked).filter(k => dlg.picked[k]))
                dlg.close()
            }
        }
    }
}
