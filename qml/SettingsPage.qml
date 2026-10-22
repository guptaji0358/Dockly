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

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: scroll.availableWidth - 14
                spacing: 14

                SettingGroup {
                    title: "Startup"
                    SettingCheck { key: "startWithWindows"; text: "Start with Windows" }
                    SettingCheck { key: "minimizeToTray"; text: "Minimize to system tray" }
                    SettingCombo {
                        key: "defaultTab"; label: "Default tab on startup"
                        options: [{text: "Home", value: "home"}, {text: "Software", value: "software"},
                                  {text: "Custom (Collections)", value: "custom"}]
                    }
                }

                SettingGroup {
                    title: "Appearance"
                    SettingCombo {
                        key: "style"; label: "Colour palette"
                        options: [{text: "Neon", value: "neon"}, {text: "Playful", value: "playful"},
                                  {text: "Editorial", value: "editorial"}]
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
                            text: "Skin default"
                            enabled: core.settings.values.accent !== ""
                            onClicked: core.settings.set("accent", "")
                        }
                    }
                }

                SettingGroup {
                    title: "Performance  (GPU)"
                    SettingCombo {
                        key: "gpuLevel"; label: "Effects level"
                        options: [{text: "Low - flat, no effects", value: "low"},
                                  {text: "Balanced - aurora, tilt, float", value: "balanced"},
                                  {text: "High - aurora, stars, parallax", value: "high"}]
                    }
                    SettingCombo {
                        key: "graphicsApi"; label: "Graphics backend"
                        options: [{text: "OpenGL", value: "opengl"}, {text: "Direct3D 11", value: "d3d11"},
                                  {text: "Software (no GPU)", value: "software"}]
                    }
                    Text {
                        text: "Graphics backend applies after restarting Dockly. Software mode turns the shader background off."
                        color: page.th.subtext
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    SettingCheck { key: "animations"; text: "Animations" }
                    SettingCheck { key: "floatingTiles"; text: "Floating tiles (idle drift + tilt toward the pointer)" }
                    SettingCheck { key: "pauseWhenInactive"; text: "Pause effects while Dockly is in the background" }
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
                    SettingCheck { key: "confirmDelete"; text: "Confirm before delete" }
                    SettingCheck { key: "checkMissingOnStartup"; text: "Check for missing items on startup" }
                }

                SettingGroup {
                    title: "General"
                    SettingCombo {
                        key: "language"; label: "Language"
                        options: [{text: "English", value: "English"}]
                    }
                }

                SettingGroup {
                    title: "Browser"
                    SettingCombo {
                        key: "defaultBrowser"; label: "Default browser"
                        options: [{text: "System default", value: ""}].concat(core.browsers())
                    }
                    Text {
                        text: "Website tiles open in this browser instead of the one set in Windows."
                        color: page.th.subtext
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                SettingGroup {
                    title: "Data"
                    Flow {
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
                    }
                }

                SettingGroup {
                    title: "About"
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        AppButton { text: "About"; onClicked: page.host.ask("About Dockly", core.aboutText(), ["OK"]) }
                        AppButton { text: "Check for Updates"; onClicked: page.host.ask("Check for Updates", core.checkForUpdates(), ["OK"]) }
                    }
                }
                Item { Layout.preferredHeight: 8 }
            }
        }
    }
}
