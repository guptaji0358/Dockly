import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Connect Dockly with other applications: a browser extension that adds pages as shortcuts,
// plus importers for browser bookmarks, GitHub repositories and VS Code projects.
Item {
    id: page
    property var host
    readonly property var th: core.theme
    readonly property var cn: core.connectors
    property string githubError: ""
    property string vscodeNote: ""

    Connections {
        target: page.cn
        function onGithubReady(entries, error) {
            page.githubError = error
            if (error === "")
                picker.openWith("GitHub · " + gh.text.trim(), entries, "public repositories", true)
        }
    }

    // one connector: icon, title, status light, and the controls as children
    component ConnectorCard: Glass {
        id: card
        property string title
        property string blurb
        property string iconName
        property string lamp: "off"          // on | off | soon
        property string stateText: ""
        default property alias body: bodyCol.data
        Layout.fillWidth: true
        implicitHeight: bodyCol.implicitHeight + 92
        radius: 20
        tint: 0.55
        brackets: card.lamp === "on"
        edge: card.lamp === "on" ? Qt.rgba(page.th.accent.r, page.th.accent.g, page.th.accent.b, 0.45) : Qt.rgba(1, 1, 1, 0.12)
        opacity: card.lamp === "soon" ? 0.6 : 1

        Rectangle {
            id: badge
            x: 20; y: 20; width: 48; height: 48; radius: 24
            color: Qt.rgba(page.th.accent.r, page.th.accent.g, page.th.accent.b, 0.16)
            border.width: 1.5; border.color: page.th.accent
            Icon { anchors.centerIn: parent; name: card.iconName; size: 24; color: page.th.accent }
        }
        Column {
            x: 84; y: 20
            width: parent.width - 84 - 180
            spacing: 2
            Text { text: card.title; color: page.th.text; font.pixelSize: 17; font.weight: Font.DemiBold }
            Text { text: card.blurb; color: page.th.subtext; font.pixelSize: 12; width: parent.width; wrapMode: Text.Wrap }
        }
        Rectangle {   // status pill
            anchors.right: parent.right
            anchors.rightMargin: 20
            y: 26
            implicitHeight: 26
            implicitWidth: st.implicitWidth + 32
            radius: 13
            readonly property color tone: card.lamp === "on" ? "#3ddc97" : (card.lamp === "soon" ? page.th.warning : page.th.subtext)
            color: Qt.rgba(tone.r, tone.g, tone.b, 0.14)
            border.width: 1; border.color: Qt.rgba(tone.r, tone.g, tone.b, 0.5)
            Rectangle { x: 10; anchors.verticalCenter: parent.verticalCenter; width: 8; height: 8; radius: 4; color: parent.tone }
            Text { id: st; x: 24; anchors.verticalCenter: parent.verticalCenter; text: card.stateText
                   color: parent.tone; font.pixelSize: 12 }
        }
        ColumnLayout {
            id: bodyCol
            x: 20; y: 84
            width: parent.width - 40
            spacing: 10
        }
    }

    component Small: Text {
        color: page.th.subtext
        font.pixelSize: 12
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Text {
            text: "Connectors"
            color: page.th.text
            font.pixelSize: 26
            font.weight: Font.DemiBold
        }
        Small { text: "Link Dockly with the apps you use. Everything stays on this PC; nothing is sent anywhere except the GitHub request you start yourself." }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: scroll.availableWidth - 14
                spacing: 14

                // ---- browser -----------------------------------------------------------
                ConnectorCard {
                    title: "Chrome · Edge · Brave"
                    blurb: "Add the page you are on, a link, or every open tab to Dockly straight from the browser."
                    iconName: "web"
                    lamp: page.cn.bridgeRunning ? "on" : "off"
                    stateText: page.cn.bridgeRunning ? "Listening on 127.0.0.1:" + page.cn.bridgePort : "Off"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Rectangle {   // switch
                            id: sw
                            width: 48; height: 26; radius: 13
                            color: page.cn.bridgeEnabled ? page.th.accent : Qt.rgba(1, 1, 1, 0.14)
                            Behavior on color { enabled: Fx.animated; ColorAnimation { duration: 140 } }
                            Rectangle {
                                y: 3; width: 20; height: 20; radius: 10; color: "white"
                                x: page.cn.bridgeEnabled ? parent.width - width - 3 : 3
                                Behavior on x { enabled: Fx.animated; NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: page.cn.bridgeEnabled = !page.cn.bridgeEnabled }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: page.cn.bridgeEnabled ? "Browser connector is on" : "Turn on to accept shortcuts from the browser extension"
                            color: page.th.text
                        }
                        Text {
                            visible: page.cn.bridgeRunning
                            text: page.cn.received + " added this session"
                            color: page.th.accent
                            font.pixelSize: 12
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: page.cn.bridgeRunning
                        spacing: 6
                        Text { text: "PAIRING CODE"; color: page.th.accent; font.pixelSize: 10; font.letterSpacing: 3; font.weight: Font.DemiBold }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            HudField {
                                Layout.fillWidth: true
                                readOnly: true
                                text: page.cn.pairingCode
                                font.family: "Consolas"
                                font.pixelSize: 13
                            }
                            OrbButton { diameter: 34; iconName: "check"; tip: "Copy code"; onClicked: page.cn.copyPairingCode() }
                            OrbButton { diameter: 34; iconName: "scan"; tip: "Make a new code (unpairs the browser)"
                                        onClicked: page.cn.regenerateToken() }
                        }
                        Small {
                            text: "1.  Open chrome://extensions (or edge://extensions) and switch on Developer mode.<br>"
                                + "2.  Choose Load unpacked and select the extension folder (button below).<br>"
                                + "3.  Click the Dockly icon in the browser, paste the pairing code once, and you're connected.<br>"
                                + "Then use the toolbar button, or right-click any page or link &gt; Add to Dockly."
                            textFormat: Text.StyledText
                        }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            AppButton { iconName: "open"; text: "Open extension folder"; onClicked: page.cn.openExtensionFolder() }
                        }
                    }
                }

                // ---- bookmarks ---------------------------------------------------------
                ConnectorCard {
                    title: "Browser bookmarks"
                    blurb: "Pick bookmarks from Chrome, Edge or Brave and pin them as web shortcuts."
                    iconName: "star"
                    lamp: page.cn.installedBrowsers().length > 0 ? "on" : "off"
                    stateText: page.cn.installedBrowsers().length > 0 ? page.cn.installedBrowsers().length + " browser(s) found" : "No browser found"

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: page.cn.installedBrowsers()
                            delegate: AppButton {
                                required property string modelData
                                text: "Import from " + (modelData === "chrome" ? "Chrome" : modelData === "edge" ? "Edge" : "Brave")
                                onClicked: picker.openWith(text.replace("Import from ", "") + " bookmarks",
                                                           page.cn.browserBookmarks(modelData), "")
                            }
                        }
                    }
                }

                // ---- GitHub --------------------------------------------------------------
                ConnectorCard {
                    title: "GitHub"
                    blurb: "Turn any user's public repositories into shortcuts. No sign-in or token needed."
                    iconName: "github"
                    lamp: page.cn.busy ? "on" : "off"
                    stateText: page.cn.busy ? "Fetching…" : "Ready"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        HudField {
                            id: gh
                            Layout.fillWidth: true
                            placeholderText: "GitHub username, e.g. guptaji0358"
                            onAccepted: fetchBtn.clicked()
                        }
                        AppButton {
                            id: fetchBtn
                            primary: true
                            iconName: "scan"
                            text: page.cn.busy ? "Fetching…" : "Fetch repos"
                            enabled: !page.cn.busy && gh.text.trim() !== ""
                            onClicked: { page.githubError = ""; page.cn.fetchGithub(gh.text) }
                        }
                    }
                    Text { visible: page.githubError !== ""; text: page.githubError; color: page.th.missing; font.pixelSize: 12 }
                }

                // ---- VS Code -------------------------------------------------------------
                ConnectorCard {
                    title: "Visual Studio Code"
                    blurb: "Pin your recent VS Code folders and workspaces. They open in VS Code itself."
                    iconName: "software"
                    lamp: "off"
                    stateText: "On demand"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        AppButton {
                            iconName: "scan"
                            text: "Find recent projects"
                            onClicked: {
                                const list = page.cn.vscodeRecent()
                                page.vscodeNote = list.length === 0 ? "No recent VS Code projects were found." : ""
                                if (list.length > 0)
                                    picker.openWith("VS Code projects", list, "opens in VS Code")
                            }
                        }
                        Text { visible: page.vscodeNote !== ""; text: page.vscodeNote; color: page.th.subtext; font.pixelSize: 12 }
                    }
                }
                Item { Layout.preferredHeight: 8 }
            }
        }
    }

    ConnectorPicker { id: picker }
}
