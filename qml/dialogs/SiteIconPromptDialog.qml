import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Shown before a site's image is chosen: a ready-made prompt that makes ChatGPT draw a clean icon
// (with the site's logo attached as reference), then the file picker.
AppDialog {
    id: dlg
    signal picked(string path)

    title: "Make a site icon"
    width: Math.min(560, (Overlay.overlay ? Overlay.overlay.width : 560) - 40)

    // One common prompt for every site: the attached logo is only converted, never redrawn.
    readonly property string prompt:
        "Convert the attached logo into a clean app icon image.\n"
        + "Keep the logo exactly as it is - same shape, colours and proportions. Do not redesign, rewrite or replace anything in it.\n"
        + "Only prepare it for use as an icon:\n"
        + "- remove the background so it is transparent\n"
        + "- centre it on a square canvas with even padding\n"
        + "- output a PNG, 1024 x 1024 px\n"
        + "- no extra text, watermark, border, mockup or frame"

    function openFor() {
        copied.visible = false
        open()
    }

    Text {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: dlg.th.subtext
        text: "1. Copy the prompt   2. Open ChatGPT, paste it and attach the site's logo (a screenshot works)   "
              + "3. Save the image ChatGPT makes   4. Choose that image here"
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 150
        radius: 10
        color: dlg.th.bg
        border.color: dlg.th.border
        ScrollView {
            anchors.fill: parent
            anchors.margins: 2
            TextArea {
                id: promptBox
                text: dlg.prompt
                readOnly: true
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                color: dlg.th.text
                font.pixelSize: 12
                background: null
            }
        }
    }

    Text {
        id: copied
        visible: false
        text: "Prompt copied. Paste it in ChatGPT."
        color: dlg.th.accent
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        AppButton {
            text: "Copy prompt"
            onClicked: {
                promptBox.selectAll()
                promptBox.copy()
                promptBox.deselect()
                copied.visible = true
            }
        }
        AppButton {
            text: "Open ChatGPT"
            onClicked: Qt.openUrlExternally("https://chatgpt.com/")
        }
        Item { Layout.fillWidth: true }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 8
        Item { Layout.fillWidth: true }
        AppButton { text: "Cancel"; onClicked: dlg.close() }
        AppButton {
            text: "Choose image..."
            primary: true
            onClicked: {
                const f = core.pickFiles("Choose site icon image", core.imageFilter(), false)
                if (f.length > 0) {
                    dlg.picked(f[0])
                    dlg.close()
                }
            }
        }
    }
}
