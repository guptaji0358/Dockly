import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Shown before a site's image is chosen: a ready-made prompt that makes ChatGPT draw a clean icon
// (with the site's logo attached as reference), then the file picker.
AppDialog {
    id: dlg
    property string siteName: ""
    property string siteHost: ""
    property string tone: ""
    signal picked(string path)

    title: "Make a site icon"
    width: Math.min(560, (Overlay.overlay ? Overlay.overlay.width : 560) - 40)

    readonly property string prompt:
        "Create a square app icon for \"" + (siteName.trim() || "my website") + "\""
        + (siteHost !== "" ? " (" + siteHost + ")" : "") + ".\n"
        + "Use the attached image as the reference for the logo or symbol and keep it recognisable.\n"
        + "Requirements:\n"
        + "- one simple, bold, centred symbol; flat or softly shaded, no tiny details\n"
        + "- transparent background, PNG, 1024 x 1024 px, square canvas with even padding\n"
        + "- no text, no watermark, no border, no mockup, no device frame\n"
        + (tone !== "" ? "- main colour close to " + tone + "\n" : "")
        + "- must stay clear and readable when shrunk to 48 x 48 px"

    function openFor(name, host, colour) {
        siteName = name
        siteHost = host
        tone = colour
        copied.visible = false
        open()
    }

    Text {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: dlg.th.subtext
        text: "1. Copy the prompt   2. Open ChatGPT, paste it and attach the site's logo (or a screenshot of it)   "
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
