import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LH

// Add / edit a single item (name + path or URL). Collections only have a name.
AppDialog {
    id: dlg
    property int category: LH.Web
    property string itemId: ""
    readonly property bool editing: itemId !== ""
    readonly property bool isFile: category !== LH.Web && category !== LH.Collection
    signal created(string newId)

    title: category === LH.Collection ? (editing ? "Rename Collection" : "Create Collection")
                                      : (editing ? "Edit Item" : "Add URL")

    function openAdd(cat) {
        category = cat
        itemId = ""
        nameField.text = ""
        targetField.text = ""
        open()
        if (category === LH.Collection)
            nameField.forceActiveFocus()
        else
            targetField.forceActiveFocus()
    }
    function openEdit(id) {
        const i = core.itemInfo(id)
        category = i.category
        itemId = id
        nameField.text = i.name
        targetField.text = i.target
        open()
    }

    component Field: TextField {
        Layout.fillWidth: true
        implicitHeight: 38
        color: dlg.th.text
        placeholderTextColor: dlg.th.subtext
        selectByMouse: true
        background: Rectangle {
            radius: 8
            color: dlg.th.bg
            border.color: parent.activeFocus ? dlg.th.accent : dlg.th.border
        }
    }

    Text { text: "Name"; color: dlg.th.subtext }
    Field {
        id: nameField
        placeholderText: dlg.category === LH.Collection ? "Collection name" : "Optional - taken from the target"
        onAccepted: save.clicked()
    }
    Text { text: dlg.category === LH.Web ? "URL" : "Path"; color: dlg.th.subtext; visible: dlg.category !== LH.Collection }
    RowLayout {
        Layout.fillWidth: true
        visible: dlg.category !== LH.Collection
        Field {
            id: targetField
            placeholderText: dlg.category === LH.Web ? "https://example.com" : "Choose a file..."
            onAccepted: save.clicked()
        }
        AppButton {
            text: "Browse..."
            visible: dlg.isFile
            onClicked: {
                const f = core.pickFiles("Choose file", core.fileFilter(dlg.category), false)
                if (f.length > 0)
                    targetField.text = f[0]
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 8
        Item { Layout.fillWidth: true }
        AppButton { text: "Cancel"; onClicked: dlg.close() }
        AppButton {
            id: save
            text: "Save"
            primary: true
            enabled: dlg.category === LH.Collection ? nameField.text.trim() !== "" : targetField.text.trim() !== ""
            onClicked: {
                if (!enabled)
                    return
                if (dlg.editing) {
                    core.updateItem(dlg.itemId, nameField.text, targetField.text)
                } else if (dlg.category === LH.Collection) {
                    dlg.created(core.createCollection(nameField.text))
                } else {
                    dlg.created(core.addItem(dlg.category, nameField.text, targetField.text))
                }
                dlg.close()
            }
        }
    }
}
