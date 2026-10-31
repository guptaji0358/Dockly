import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Run with arguments": asks for command-line arguments and starts the program once with them.
AppDialog {
    id: dlg
    property string itemId
    property string itemName

    function openFor(id, name) {
        itemId = id
        itemName = name
        field.text = ""
        title = "Run with arguments"
        open()
        field.forceActiveFocus()
    }
    function run() {
        const ok = core.launchWithArgs(itemId, field.text)
        close()
        if (ok)
            core.message("Started \"" + itemName + "\".")
    }

    Text {
        Layout.fillWidth: true
        text: "Arguments passed to \"" + dlg.itemName + "\" this time only (nothing is saved)."
        color: dlg.th.subtext
        wrapMode: Text.Wrap
    }
    HudField {
        id: field
        Layout.fillWidth: true
        placeholderText: "e.g. --incognito  \"C:\\notes\\todo.txt\""
        onAccepted: dlg.run()
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 8
        Item { Layout.fillWidth: true }
        AppButton { text: "Cancel"; onClicked: dlg.close() }
        AppButton { text: "Run"; primary: true; onClicked: dlg.run() }
    }
}
