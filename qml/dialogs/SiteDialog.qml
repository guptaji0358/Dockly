import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Add / edit one site rule: which hosts it covers and how its links are grouped.
AppDialog {
    id: dlg
    property string siteId: ""
    readonly property bool editing: siteId !== ""
    property string color: "#4C8DFF"
    property string icon: "web"
    property int groupIndex: 0
    property string image: ""        // file URL of the picture the site has now
    property string pickedImage: ""  // newly chosen file, copied in on Save
    property bool removeImage: false
    readonly property string shownImage: pickedImage !== "" ? "file:///" + pickedImage.replace(/\\/g, "/")
                                                            : (removeImage ? "" : image)
    readonly property var groupOptions: [
        {text: "One group for the whole site", value: "none"},
        {text: "By first folder in the link (e.g. GitHub owner)", value: "path"},
        {text: "By subdomain (e.g. docs.example.com)", value: "subdomain"}
    ]
    readonly property var palette: ["#4C8DFF", "#FFA94D", "#F06595", "#3DDC97", "#A78BFA", "#22D3EE", "#FACC15", "#FB7185", "#10A37F", "#E2E8F0"]

    title: editing ? "Edit site" : "New site"

    function openNew() {
        siteId = ""
        nameField.text = ""
        hostsField.text = ""
        groupIndex = 0
        autoBox.checked = true
        color = palette[Math.floor(Math.random() * 8)]
        icon = "web"
        image = ""
        pickedImage = ""
        removeImage = false
        open()
        nameField.forceActiveFocus()
    }
    function openEdit(site) {
        siteId = site.id
        nameField.text = site.name
        hostsField.text = site.hostsText
        groupIndex = Math.max(0, groupOptions.findIndex(o => o.value === site.groupBy))
        autoBox.checked = site.autoFile
        color = site.color
        icon = site.icon
        image = site.image
        pickedImage = ""
        removeImage = false
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
    Field { id: nameField; placeholderText: "e.g. ChatGPT" }

    Text { text: "Links on these websites belong to it"; color: dlg.th.subtext }
    Field { id: hostsField; placeholderText: "chatgpt.com, chat.openai.com" }

    Text { text: "Group its links"; color: dlg.th.subtext }
    ComboBox {
        id: combo
        Layout.fillWidth: true
        implicitHeight: 38
        model: dlg.groupOptions
        textRole: "text"
        currentIndex: dlg.groupIndex
        onActivated: index => dlg.groupIndex = index
        contentItem: Text {
            text: combo.displayText
            color: dlg.th.text
            verticalAlignment: Text.AlignVCenter
            leftPadding: 12
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 8
            color: dlg.th.bg
            border.color: combo.activeFocus || combo.hovered ? dlg.th.accent : dlg.th.border
        }
        indicator: Text {
            x: combo.width - width - 12
            y: (combo.height - height) / 2
            text: "▾"
            color: dlg.th.subtext
        }
        delegate: ItemDelegate {
            id: opt
            required property int index
            required property var modelData
            width: combo.width
            contentItem: Text {
                text: opt.modelData.text
                color: opt.highlighted ? "white" : dlg.th.text
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { color: opt.highlighted ? dlg.th.accent : dlg.th.surface }
            highlighted: combo.highlightedIndex === index
        }
        popup: Popup {
            y: combo.height + 2
            width: combo.width
            implicitHeight: contentItem.implicitHeight + 2
            padding: 1
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
            }
            background: Rectangle { color: dlg.th.surface; border.color: dlg.th.border; radius: 8 }
        }
    }

    CheckBox {
        id: autoBox
        checked: true
        spacing: 10
        text: "File new links into it automatically"
        contentItem: Text {
            text: autoBox.text
            color: dlg.th.text
            verticalAlignment: Text.AlignVCenter
            leftPadding: autoBox.indicator.width + autoBox.spacing
        }
        indicator: Rectangle {
            implicitWidth: 18
            implicitHeight: 18
            y: (autoBox.height - height) / 2
            radius: 5
            color: autoBox.checked ? dlg.th.accent : dlg.th.surface
            border.width: 1.5
            border.color: autoBox.checked || autoBox.hovered ? dlg.th.accent : dlg.th.subtext
            Text {
                anchors.centerIn: parent
                visible: autoBox.checked
                text: "✓"
                color: "white"
                font.pixelSize: 13
                font.bold: true
            }
        }
    }

    Text { text: "Colour"; color: dlg.th.subtext }
    Flow {
        Layout.fillWidth: true
        spacing: 8
        Repeater {
            model: dlg.palette
            delegate: Rectangle {
                required property string modelData
                width: 28
                height: 28
                radius: 14
                color: modelData
                border.width: dlg.color === modelData ? 3 : 0
                border.color: dlg.th.text
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dlg.color = parent.modelData
                }
            }
        }
    }

    Text { text: "Icon"; color: dlg.th.subtext }
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Rectangle {
            width: 52
            height: 52
            radius: 12
            color: Qt.alpha(dlg.color, 0.22)
            SiteIcon {
                anchors.centerIn: parent
                image: dlg.shownImage
                name: dlg.icon
                size: 34
                color: dlg.color
            }
        }
        AppButton {
            text: dlg.shownImage === "" ? "Use own image..." : "Change image..."
            onClicked: iconPrompt.openFor()
        }
        AppButton {
            visible: dlg.shownImage !== ""
            text: "Use default icon"
            onClicked: { dlg.pickedImage = ""; dlg.removeImage = true }
        }
        Item { Layout.fillWidth: true }
    }

    SiteIconPromptDialog {
        id: iconPrompt
        onPicked: path => { dlg.pickedImage = path; dlg.removeImage = false }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 8
        AppButton {
            visible: dlg.editing
            text: "Delete site"
            danger: true
            onClicked: { core.sites.removeSite(dlg.siteId); dlg.close() }
        }
        Item { Layout.fillWidth: true }
        AppButton { text: "Cancel"; onClicked: dlg.close() }
        AppButton {
            text: "Save"
            primary: true
            enabled: nameField.text.trim() !== "" && hostsField.text.trim() !== ""
            onClicked: {
                const ok = core.sites.saveSite({
                    id: dlg.siteId, name: nameField.text, hosts: hostsField.text,
                    groupBy: dlg.groupOptions[dlg.groupIndex].value, autoFile: autoBox.checked,
                    icon: dlg.icon, color: dlg.color,
                    pickedImage: dlg.pickedImage, removeImage: dlg.removeImage
                })
                if (ok)
                    dlg.close()
            }
        }
    }
}
