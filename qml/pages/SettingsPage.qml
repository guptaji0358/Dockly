import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Every option from the Settings tab of the flowchart.
Item {
    id: page
    property var host
    readonly property var th: core.theme

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Text {
            text: "Settings"
            color: page.th.text
            font.pixelSize: 26
            font.weight: Font.DemiBold
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

        ColumnLayout {
            Layout.preferredWidth: 170
            Layout.alignment: Qt.AlignTop
            spacing: 4
            Repeater {
                model: ["General", "Dock", "Appearance", "Items", "Performance", "Data & About"]
                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    readonly property bool current: stack.currentIndex === index
                    Layout.fillWidth: true
                    implicitHeight: 38
                    radius: 8
                    color: current ? page.th.accent : (hover.hovered ? page.th.surface : "transparent")
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        text: parent.modelData
                        color: parent.current ? "white" : page.th.text
                        font.pixelSize: 14
                        font.weight: parent.current ? Font.DemiBold : Font.Normal
                    }
                    HoverHandler { id: hover }
                    TapHandler { onTapped: stack.currentIndex = parent.index }
                }
            }
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            StackLayout {
                id: stack
                width: scroll.availableWidth - 14

                SettingGroup {
                    title: "General"
                    SettingCheck { key: "startWithWindows"; text: "Start with Windows" }
                    SettingCheck { key: "minimizeToTray"; text: "Minimize to system tray" }
                    SettingCombo {
                        key: "defaultTab"; label: "Default tab"
                        options: [{text: "Home", value: "home"}, {text: "Software", value: "software"},
                                  {text: "Custom (Collections)", value: "custom"}]
                    }
                    SettingCombo {
                        key: "defaultBrowser"; label: "Default browser"
                        options: [{text: "System default", value: ""}].concat(core.browsers())
                    }
                    SettingCombo {
                        key: "language"; label: "Language"
                        options: [{text: "English", value: "English"}]
                    }
                    SettingCheck { key: "launcherEnabled"; text: "Quick launcher: type-to-launch box from anywhere" }
                    SettingCombo {
                        key: "launcherHotkey"; label: "Quick launcher shortcut"
                        options: [{text: "Alt + Space", value: "Alt+Space"}, {text: "Ctrl + Space", value: "Ctrl+Space"},
                                  {text: "Ctrl + Alt + Space", value: "Ctrl+Alt+Space"}, {text: "Ctrl + Shift + Space", value: "Ctrl+Shift+Space"},
                                  {text: "Win + Space", value: "Meta+Space"}, {text: "None", value: ""}]
                    }
                }

                SettingGroup {
                    title: "Dock"
                    SettingCheck { key: "dockEnabled"; text: "Dock Dockly to the screen edge on launch" }
                    SettingCombo {
                        key: "dockMode"; label: "Mode"
                        options: [{text: "Categories", value: "categories"}, {text: "Favourites", value: "favorites"}]
                    }
                    SettingCombo {
                        key: "dockEdge"; label: "Position"
                        options: [{text: "Right", value: "right"}, {text: "Left", value: "left"},
                                  {text: "Top", value: "top"}]
                    }
                    SettingCombo {
                        key: "dockHotkey"; label: "Shortcut"
                        options: [{text: "Ctrl + Alt + D", value: "Ctrl+Alt+D"}, {text: "Ctrl + Alt + Space", value: "Ctrl+Alt+Space"},
                                  {text: "Ctrl + Shift + Space", value: "Ctrl+Shift+Space"}, {text: "Win + Shift + D", value: "Meta+Shift+D"},
                                  {text: "None", value: ""}]
                    }
                    SettingCombo {
                        key: "dockOpacity"; label: "Background"
                        options: [{text: "Transparent", value: 0}, {text: "Very light", value: 20}, {text: "Light", value: 40},
                                  {text: "Medium", value: 70}, {text: "Solid", value: 100}]
                    }
                    SettingCombo {
                        key: "dockCollapseDelay"; label: "Collapse delay"
                        customMin: 300; customMax: 10000; customDefault: 1000; customSuffix: " ms"
                        options: [{text: "Instant", value: 0}, {text: "Quick (0.1 s)", value: 100}, {text: "Normal (0.2 s)", value: 200},
                                  {text: "Relaxed (0.4 s)", value: 400}, {text: "Slow (0.8 s)", value: 800},
                                  {text: "Custom…", value: -1}]
                    }
                    SettingCombo {
                        key: "dockIdleHide"; label: "Hide the line when idle"
                        customMin: 2; customDefault: 120; customSuffix: " seconds"
                        options: [{text: "Never", value: 0}, {text: "After 3 seconds", value: 3}, {text: "After 10 seconds", value: 10},
                                  {text: "After 30 seconds", value: 30}, {text: "After 1 minute", value: 60},
                                  {text: "After 5 minutes", value: 300}, {text: "Custom…", value: -1}]
                    }
                    SettingCheck { key: "dockAutoHide"; text: "Auto-hide after launching an app" }
                    SettingCheck { key: "dockHideFullscreen"; text: "Hide in full-screen apps (rest the pointer on the edge to show)" }
                    Text {
                        text: "With the dock off, open the full window from the tray icon."
                        color: page.th.subtext
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                SettingGroup {
                    title: "Appearance"
                    SettingCombo {
                        key: "style"; label: "Skin"
                        options: [{text: "Neon", value: "neon"}, {text: "Playful", value: "playful"},
                                  {text: "Editorial", value: "editorial"}]
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: "Colour theme"; color: page.th.text; Layout.preferredWidth: 170 }
                        Repeater {
                            model: page.th.palettes()
                            delegate: Item {
                                id: sw
                                required property var modelData
                                readonly property bool on: core.settings.values.palette === modelData.key
                                implicitWidth: 52
                                implicitHeight: 62
                                Rectangle {
                                    id: dot
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 34; height: 34; radius: 17
                                    // "Skin default" shows the skin's own accent
                                    color: sw.modelData.accent !== "" ? sw.modelData.accent : page.th.accent
                                    border.width: sw.on ? 3 : 1
                                    border.color: sw.on ? page.th.text : page.th.border
                                    scale: swMouse.containsMouse ? 1.1 : 1
                                    Behavior on scale { NumberAnimation { duration: 90 } }
                                }
                                Text {
                                    anchors.top: dot.bottom
                                    anchors.topMargin: 4
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: sw.modelData.label
                                    color: sw.on ? page.th.text : page.th.subtext
                                    font.pixelSize: 11
                                    font.weight: sw.on ? Font.DemiBold : Font.Normal
                                }
                                MouseArea {
                                    id: swMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: core.settings.set("palette", sw.modelData.key)
                                }
                            }
                        }
                    }
                    SettingCombo {
                        key: "layoutMode"; label: "Layout"
                        options: [{text: "Honeycomb", value: "hex"}, {text: "Orbit", value: "orbit"},
                                  {text: "Classic grid", value: "grid"}]
                    }
                    SettingCombo {
                        key: "theme"; label: "Theme"
                        options: [{text: "System", value: "system"}, {text: "Light", value: "light"},
                                  {text: "Dark", value: "dark"}]
                    }
                    RowLayout {
                        spacing: 16
                        Text { text: "Accent colour"; color: page.th.text; Layout.preferredWidth: 170 }
                        AppButton {
                            Layout.preferredWidth: 240
                            primary: true
                            text: page.th.accent.toString()
                            onClicked: {
                                const c = core.pickColor(page.th.accent.toString())
                                if (c !== "")
                                    core.settings.set("accent", c)
                            }
                        }
                        AppButton {
                            text: "Theme default"
                            enabled: core.settings.values.accent !== ""
                            onClicked: core.settings.set("accent", "")
                        }
                    }
                }

                SettingGroup {
                    title: "Items"
                    SettingCheck { key: "recentEnabled"; text: "Recent items" }
                    RowLayout {
                        spacing: 16
                        Text { text: "Max recent items"; color: page.th.text; Layout.preferredWidth: 170 }
                        SpinBox {
                            id: maxRecent
                            from: 1
                            to: 100
                            editable: true
                            value: core.settings.values.maxRecent
                            onValueModified: core.settings.set("maxRecent", value)
                            Connections {
                                target: core.settings
                                function onValuesChanged() { maxRecent.value = core.settings.values.maxRecent }
                            }
                        }
                    }
                    SettingCombo {
                        key: "workspaceDelay"; label: "Workspace launch spacing"
                        customMin: 50; customMax: 10000; customDefault: 1000; customSuffix: " ms"
                        options: [{text: "All at once", value: 0}, {text: "Quick (0.2 s)", value: 200}, {text: "Normal (0.4 s)", value: 400},
                                  {text: "Relaxed (1 s)", value: 1000}, {text: "Slow (2 s)", value: 2000}, {text: "Custom…", value: -1}]
                    }
                    SettingCheck { key: "confirmDelete"; text: "Confirm before delete" }
                    SettingCheck { key: "checkMissingOnStartup"; text: "Check for missing items on startup" }
                }

                SettingGroup {
                    title: "Performance"
                    SettingCombo {
                        key: "gpuLevel"; label: "Effects level"
                        options: [{text: "Low", value: "low"},
                                  {text: "Balanced", value: "balanced"},
                                  {text: "High", value: "high"}]
                    }
                    SettingCombo {
                        key: "graphicsApi"; label: "Graphics backend"
                        options: [{text: "OpenGL", value: "opengl"}, {text: "Direct3D 11", value: "d3d11"},
                                  {text: "Software (no GPU)", value: "software"}]
                    }
                    Text {
                        text: "Graphics backend applies after a restart."
                        color: page.th.subtext
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    SettingCheck { key: "animations"; text: "Animations" }
                    SettingCheck { key: "floatingTiles"; text: "Floating tiles" }
                    SettingCheck { key: "pauseWhenInactive"; text: "Pause effects in the background" }
                }

                SettingGroup {
                    title: "Data & About"
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        AppButton {
                            text: "Backup Data (Export JSON)"
                            onClicked: {
                                const p = core.pickSaveFile("Backup data", core.defaultBackupPath(), "JSON (*.json)")
                                if (p !== "")
                                    core.backupTo(p)
                            }
                        }
                        AppButton {
                            text: "Restore Data (Import JSON)"
                            onClicked: {
                                const f = core.pickFiles("Restore data", "JSON (*.json)", false)
                                if (f.length === 0)
                                    return
                                page.host.ask("Restore data",
                                              "This replaces all current items and settings with the backup. Continue?",
                                              ["Restore", "Cancel"], i => { if (i === 0) core.restoreFrom(f[0]) })
                            }
                        }
                        AppButton { text: "Open Backup Folder"; onClicked: core.openBackupFolder() }
                        AppButton { text: "Clear Cache"; onClicked: core.clearCache() }
                        AppButton {
                            text: "Reset All Settings"
                            danger: true
                            onClicked: page.host.ask("Reset settings", "Restore every setting to its default?",
                                                     ["Reset", "Cancel"], i => { if (i === 0) core.resetSettings() }, 0)
                        }
                        AppButton { text: "About"; onClicked: page.host.ask("About Dockly", core.aboutText(), ["OK"]) }
                        AppButton { text: "Check for Updates"; onClicked: page.host.ask("Check for Updates", core.checkForUpdates(), ["OK"]) }
                    }
                }
            }
        }
        }
    }
}
