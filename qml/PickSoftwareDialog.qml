import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Searchable list of programs from the hidden catalog; only ticked ones become cards.
AppDialog {
    id: dlg
    property var entries: []
    property var picked: ({})   // target -> name
    readonly property int total: core.catalogCount

    title: "Add Software"
    width: Math.min(560, (Overlay.overlay ? Overlay.overlay.width : 560) - 40)

    function openPicker() {
        picked = ({})
        search.text = ""
        reload()
        open()
    }
    function reload() { entries = core.catalogSearch(search.text, 300) }

    Connections {
        target: core
        function onCatalogChanged() { dlg.reload() }
    }

    TextField {
        id: search
        Layout.fillWidth: true
        implicitHeight: 38
        placeholderText: "Search installed programs..."
        color: dlg.th.text
        placeholderTextColor: dlg.th.subtext
        onTextChanged: dlg.reload()
        background: Rectangle {
            radius: dlg.th.radius === 0 ? 0 : 8
            color: dlg.th.bg
            border.color: search.activeFocus ? dlg.th.accent : dlg.th.border
        }
    }
    ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: 340
        clip: true
        model: dlg.entries
        ScrollBar.vertical: ScrollBar {}
        delegate: CheckDelegate {
            required property var modelData
            width: ListView.view.width
            checked: dlg.picked[modelData.target] !== undefined
            onToggled: {
                const p = dlg.picked
                if (checked)
                    p[modelData.target] = modelData.name
                else
                    delete p[modelData.target]
                dlg.picked = p
            }
            contentItem: Text {
                text: modelData.name
                color: dlg.th.text
                verticalAlignment: Text.AlignVCenter
                leftPadding: 4
                elide: Text.ElideRight
            }
            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: modelData.target
        }
    }
    Text {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: dlg.th.subtext
        text: dlg.total === 0
              ? "The catalog is empty. Press \"Scan this PC\" to look through your Start Menu shortcuts (this never scans whole drives), or browse for an .exe yourself."
              : dlg.total + " program(s) in the catalog" + (dlg.entries.length >= 300 ? " (showing the first 300, search to narrow)" : "") + ". Tick the ones you want as cards."
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        AppButton {
            iconName: "scan"
            text: core.scanning ? "Scanning..." : "Scan this PC"
            enabled: !core.scanning
            onClicked: core.refreshSoftware()
        }
        AppButton {
            text: "Browse for .exe..."
            onClicked: {
                const f = core.pickFiles("Add Software", core.fileFilter(0), true)
                if (f.length > 0) {
                    core.addFiles(0, f)
                    dlg.close()
                }
            }
        }
        Item { Layout.fillWidth: true }
        AppButton { text: "Cancel"; onClicked: dlg.close() }
        AppButton {
            text: "Add Selected"
            primary: true
            onClicked: {
                const list = []
                for (const t in dlg.picked)
                    list.push({target: t, name: dlg.picked[t]})
                if (list.length > 0)
                    core.addSoftware(list)
                dlg.close()
            }
        }
    }
}
