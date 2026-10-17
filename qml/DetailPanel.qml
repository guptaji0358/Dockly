import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LH

// Side panel for the selected card: full editing, open, favorite, locate, delete.
Rectangle {
    id: panel
    property var host
    property var info: ({})
    property bool dirty: false
    readonly property var th: core.theme
    readonly property bool collection: info.category === LH.Collection
    readonly property bool web: info.category === LH.Web

    readonly property bool shown: info.name !== undefined
    implicitWidth: 300
    radius: th.radius === 0 ? 0 : Math.min(th.radius, 18)
    color: th.surface
    border.width: Math.max(1, th.borderWidth)
    border.color: th.border

    function load() {
        const i = core.selectedId === "" ? ({}) : core.itemInfo(core.selectedId)
        info = i
        dirty = false
        if (i.name !== undefined) {
            nameField.text = i.name
            targetField.text = i.target
        }
    }
    function refresh() {
        if (core.selectedId === "")
            return
        const i = core.itemInfo(core.selectedId)
        if (i.name === undefined) {
            core.select("")
            return
        }
        info = i
        if (!dirty) {
            nameField.text = i.name
            targetField.text = i.target
        }
    }
    Component.onCompleted: load()
    Connections {
        target: core
        function onSelectionChanged() { panel.load() }
        function onItemsChanged() { panel.refresh() }
    }

    component Field: TextField {
        Layout.fillWidth: true
        implicitHeight: 38
        color: panel.th.text
        selectByMouse: true
        background: Rectangle {
            radius: panel.th.radius === 0 ? 0 : 8
            color: panel.th.bg
            border.width: panel.th.style === "editorial" ? 2 : 1
            border.color: parent.activeFocus ? panel.th.accent : panel.th.border
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Image {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                source: panel.info.id === undefined ? "" : "image://item/" + panel.info.id + "?v=" + core.iconVersion
                sourceSize: Qt.size(128, 128)
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            Text {
                Layout.fillWidth: true
                text: (panel.info.categoryName || "") + (panel.collection ? "  •  " + panel.info.memberCount + " item(s)" : "")
                color: panel.th.subtext
                wrapMode: Text.Wrap
            }
            AppButton {
                text: "✕"
                implicitWidth: 32
                leftPadding: 0
                rightPadding: 0
                Layout.alignment: Qt.AlignTop
                onClicked: core.select("")
            }
        }

        Text { text: "Name"; color: panel.th.subtext }
        Field { id: nameField; onTextEdited: panel.dirty = true }

        Text { text: panel.web ? "URL" : "Path"; color: panel.th.subtext; visible: !panel.collection }
        Field { id: targetField; visible: !panel.collection; onTextEdited: panel.dirty = true }
        AppButton {
            Layout.fillWidth: true
            visible: !panel.collection && !panel.web
            text: "Browse..."
            onClicked: {
                const f = core.pickFiles("Choose file", core.fileFilter(panel.info.category), false)
                if (f.length > 0) {
                    targetField.text = f[0]
                    panel.dirty = true
                }
            }
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: text !== ""
            text: panel.collection ? ""
                  : panel.info.status === 2 ? "⚠ Missing: nothing was found at this path."
                  : panel.info.status === 1 ? "⚠ This URL looks invalid."
                  : "✓ Available"
            color: panel.info.status === 2 ? panel.th.missing : (panel.info.status === 1 ? panel.th.warning : panel.th.subtext)
        }

        Item { Layout.preferredHeight: 4 }
        AppButton {
            Layout.fillWidth: true
            primary: true
            iconName: "open"
            text: panel.collection ? "Launch All" : "Open"
            onClicked: panel.host.launch(panel.info.id)
        }
        AppButton {
            Layout.fillWidth: true
            iconName: "check"
            text: "Save changes"
            enabled: panel.dirty
            onClicked: {
                if (core.updateItem(panel.info.id, nameField.text, targetField.text))
                    panel.load()
            }
        }
        AppButton {
            Layout.fillWidth: true
            text: panel.info.favorite ? "★  Remove from Favorites" : "☆  Add to Favorites"
            onClicked: core.setFavorite(panel.info.id, !panel.info.favorite)
        }
        AppButton {
            Layout.fillWidth: true
            visible: panel.info.status !== 0 && !panel.web && !panel.collection
            iconName: "locate"
            text: "Locate Again..."
            onClicked: {
                const f = core.pickFiles("Locate " + nameField.text, core.fileFilter(panel.info.category), false)
                if (f.length > 0) {
                    core.updateItem(panel.info.id, nameField.text, f[0])
                    panel.load()
                }
            }
        }
        Item { Layout.fillHeight: true }
        AppButton {
            Layout.fillWidth: true
            danger: true
            iconName: "trash"
            text: "Delete"
            onClicked: panel.host.remove(panel.info.id)
        }
    }
}
