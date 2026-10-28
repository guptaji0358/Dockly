import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Your imported GitHub repositories, one clean section per group (category) you created.
Item {
    id: page
    property var host
    readonly property var th: core.theme
    property var groups: []
    property string filterText: ""
    // follows the Grid / List / Compact buttons and the card-size slider
    readonly property string style: core.settings.values.viewStyle
    readonly property bool asGrid: style === "grid"
    readonly property real cardW: core.settings.values.cardSize === "small" ? 210
                                 : (core.settings.values.cardSize === "large" ? 320 : 260)

    function refresh() { groups = core.connectors.githubGroups() }
    Component.onCompleted: refresh()
    Connections {
        target: core
        function onItemsChanged() { Qt.callLater(page.refresh) }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "GitHub"
                color: page.th.text
                font.pixelSize: 26
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Glass {
                Layout.preferredWidth: 240
                Layout.preferredHeight: 38
                radius: 19
                tint: 0.5
                border.color: find.activeFocus ? page.th.accent : edge
                Icon { x: 13; anchors.verticalCenter: parent.verticalCenter; name: "search"; size: 15; color: page.th.subtext }
                TextField {
                    id: find
                    anchors.fill: parent
                    leftPadding: 38
                    rightPadding: 12
                    placeholderText: "Find a repo…"
                    color: page.th.text
                    placeholderTextColor: page.th.subtext
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    background: null
                    onTextChanged: page.filterText = text.trim().toLowerCase()
                }
            }
            AppButton { primary: true; iconName: "add"; text: "Import repos"; onClicked: page.host.goTo(page.host.tabs.findIndex(t => t.name === "Connectors")) }
        }
        Text {
            Layout.fillWidth: true
            text: "Repos you imported from GitHub, grouped the way you chose. Click a repo to open it."
            color: page.th.subtext
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: scroll.availableWidth - 14
                spacing: 18

                Repeater {
                    model: page.groups
                    delegate: ColumnLayout {
                        id: grp
                        required property var modelData
                        readonly property var shown: modelData.repos.filter(r =>
                            page.filterText === "" || (r.name + " " + r.path).toLowerCase().indexOf(page.filterText) >= 0)
                        visible: shown.length > 0
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text {
                                text: grp.modelData.name.toUpperCase()
                                color: page.th.accent
                                font.pixelSize: 13
                                font.letterSpacing: 4
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: grp.shown.length + (grp.shown.length === 1 ? " repo" : " repos")
                                color: page.th.subtext
                                font.pixelSize: 12
                            }
                            Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }
                            AppButton { text: "Open all"; iconName: "open"; onClicked: core.launchAll(grp.modelData.id) }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: page.asGrid ? 10 : (page.style === "compact" ? 2 : 6)
                            Repeater {
                                model: grp.shown
                                delegate: Glass {
                                    id: tile
                                    required property var modelData
                                    width: page.asGrid ? page.cardW : grp.width
                                    height: page.asGrid ? 64 : (page.style === "compact" ? 34 : 52)
                                    radius: page.asGrid ? 16 : (page.style === "compact" ? 8 : 12)
                                    tint: 0.5
                                    border.color: tm.containsMouse ? page.th.accent : edge
                                    Icon { x: 16; anchors.verticalCenter: parent.verticalCenter
                                           name: "github"; size: page.style === "compact" ? 16 : 22; color: page.th.accent }
                                    Column {
                                        x: page.style === "compact" ? 42 : 54
                                        width: parent.width - x - 14
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            width: parent.width
                                            text: tile.modelData.name
                                            color: page.th.text
                                            font.pixelSize: 14
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            visible: page.style !== "compact"
                                            width: parent.width
                                            text: tile.modelData.path
                                            color: page.th.subtext
                                            font.pixelSize: 11
                                            elide: Text.ElideMiddle
                                        }
                                    }
                                    MouseArea {
                                        id: tm
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: page.host.launch(tile.modelData.id)
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    visible: page.groups.length === 0
                    Layout.fillWidth: true
                    Layout.topMargin: 30
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: page.th.subtext
                    text: "No GitHub groups yet.\nIn Connectors, fetch a user's repos, tick the ones you want, give the category a name and press Add."
                }
                Item { Layout.preferredHeight: 8 }
            }
        }
    }
}
