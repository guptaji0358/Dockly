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

    // Icon: the automatic one is always shown first; a custom image only replaces it when the user picks one.
    property string pickedIcon: ""     // image chosen in this dialog, not saved yet
    property bool hadIcon: false       // the item already has a custom icon
    property bool removeIcon: false    // user went back to the original
    property string previewTarget: ""  // target the "original" preview is built from (add mode)
    readonly property string customSource: pickedIcon !== "" ? "file:///" + pickedIcon.split("\\").join("/")
        : (hadIcon && !removeIcon ? "image://item/" + itemId + "?v=" + core.iconVersion : "")
    readonly property string originalSource: editing ? "image://item/orig:" + itemId + "?v=" + core.iconVersion
        : (category === LH.Collection ? "image://item/new:" + category + ":x?v=" + core.iconVersion
           : (previewTarget === "" ? "" : "image://item/new:" + category + ":" + encodeURIComponent(previewTarget)
              + "?v=" + core.iconVersion))
    function resetIcon(had) {
        pickedIcon = ""
        removeIcon = false
        hadIcon = had
    }

    title: category === LH.Collection ? (editing ? "Rename Collection" : "Create Collection")
                                      : (editing ? "Edit Item" : (category === LH.Web ? "Add URL" : "Add Item"))

    function openAdd(cat) {
        category = cat
        itemId = ""
        resetIcon(false)
        previewTarget = ""
        nameField.text = ""
        targetField.text = ""
        open()
        if (category === LH.Collection)
            nameField.forceActiveFocus()
        else
            targetField.forceActiveFocus()
    }
    // Add dialog with the target already chosen (file picked, program selected...)
    function openAddTarget(cat, target, name) {
        openAdd(cat)
        targetField.text = target
        previewTarget = target
        nameField.text = name || ""
        nameField.forceActiveFocus()
    }
    function openEdit(id) {
        const i = core.itemInfo(id)
        category = i.category
        itemId = id
        nameField.text = i.name
        targetField.text = i.target
        resetIcon(i.icon !== "")
        previewTarget = i.target
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
            onEditingFinished: dlg.previewTarget = text
        }
        AppButton {
            text: "Browse..."
            visible: dlg.isFile
            onClicked: {
                const f = core.pickFiles("Choose file", core.fileFilter(dlg.category), false)
                if (f.length > 0) {
                    targetField.text = f[0]
                    dlg.previewTarget = f[0]
                }
            }
        }
    }
    Text { text: "Icon"; color: dlg.th.subtext }
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        component IconBox: Rectangle {
            property alias source: img.source
            property string caption
            implicitWidth: 64
            implicitHeight: 64
            radius: 12
            color: dlg.th.bg
            border.color: dlg.th.border
            Image {
                id: img
                anchors.centerIn: parent
                width: 48
                height: 48
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(96, 96)
                asynchronous: true
                smooth: true
                mipmap: true
            }
            Text {
                anchors.top: parent.bottom
                anchors.topMargin: 3
                anchors.horizontalCenter: parent.horizontalCenter
                text: parent.caption
                color: dlg.th.subtext
                font.pixelSize: 11
            }
        }
        IconBox { source: dlg.originalSource; caption: "Original" }
        Text {
            visible: dlg.customSource !== ""
            text: "→"
            color: dlg.th.subtext
            font.pixelSize: 20
        }
        IconBox { visible: dlg.customSource !== ""; source: dlg.customSource; caption: "Your icon" }
        Item { Layout.fillWidth: true }
        ColumnLayout {
            spacing: 6
            AppButton {
                text: dlg.customSource === "" ? "Use own icon..." : "Change icon..."
                onClicked: {
                    const f = core.pickFiles("Choose icon image", core.imageFilter(), false)
                    if (f.length > 0) {
                        dlg.pickedIcon = f[0]
                        dlg.removeIcon = false
                    }
                }
            }
            AppButton {
                visible: dlg.customSource !== ""
                text: "Use original"
                onClicked: {
                    dlg.pickedIcon = ""
                    dlg.removeIcon = true
                }
            }
        }
    }
    Item { implicitHeight: 12 }
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
                let id = dlg.itemId
                if (dlg.editing) {
                    core.updateItem(id, nameField.text, targetField.text)
                } else {
                    id = dlg.category === LH.Collection ? core.createCollection(nameField.text)
                                                        : core.addItem(dlg.category, nameField.text, targetField.text)
                }
                if (id !== "") {
                    if (dlg.pickedIcon !== "")
                        core.setItemIcon(id, dlg.pickedIcon)
                    else if (dlg.removeIcon)
                        core.clearItemIcon(id)
                }
                if (!dlg.editing)
                    dlg.created(id)
                dlg.close()
            }
        }
    }
}
