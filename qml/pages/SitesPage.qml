import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Websites you keep many links on (GitHub, ChatGPT, ...). Each site is a rule you can edit; its links are
// filed into groups automatically and shown here, one clean section per group.
Item {
    id: page
    property var host
    readonly property var th: core.theme
    property var sitesList: []
    property int sel: -1
    property var groups: []
    property var suggestions: []
    property int looseN: 0
    property string filterText: ""
    readonly property var cur: sel >= 0 && sel < sitesList.length ? sitesList[sel] : null
    // follows the Grid / List / Compact buttons and the card-size slider
    readonly property string style: core.settings.values.viewStyle
    readonly property bool asGrid: style === "grid"
    readonly property real cardW: core.settings.values.cardSize === "small" ? 210
                                 : (core.settings.values.cardSize === "large" ? 320 : 260)

    function refresh() {
        sitesList = core.sites.list
        if (sel < 0 || sel >= sitesList.length)
            sel = Math.max(0, sitesList.findIndex(s => s.links > 0))
        groups = cur ? core.sites.groups(cur.index) : []
        suggestions = core.sites.suggestions()
        looseN = core.sites.looseCount()
    }
    Component.onCompleted: refresh()
    Connections {
        target: core
        function onItemsChanged() { Qt.callLater(page.refresh) }
    }
    Connections {
        target: core.sites
        function onChanged() { Qt.callLater(page.refresh) }
    }
    onSelChanged: Qt.callLater(refresh)

    SiteDialog { id: siteDialog }

    component Banner: Glass {
        id: ban
        property string message
        property string actionText
        property string dismissText: "Not now"
        property bool showDismiss: true
        signal accepted()
        signal dismissed()
        Layout.fillWidth: true
        Layout.preferredHeight: 52
        radius: 14
        tint: 0.6
        border.color: Qt.rgba(page.th.accent.r, page.th.accent.g, page.th.accent.b, 0.5)
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 10
            spacing: 10
            Text {
                Layout.fillWidth: true
                text: ban.message
                color: page.th.text
                font.pixelSize: 13
                elide: Text.ElideRight
            }
            AppButton { visible: ban.showDismiss; text: ban.dismissText; onClicked: ban.dismissed() }
            AppButton { primary: true; text: ban.actionText; onClicked: ban.accepted() }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "Sites"
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
                    placeholderText: "Find a link…"
                    color: page.th.text
                    placeholderTextColor: page.th.subtext
                    verticalAlignment: TextInput.AlignVCenter
                    selectByMouse: true
                    background: null
                    onTextChanged: page.filterText = text.trim().toLowerCase()
                }
            }
            AppButton { primary: true; iconName: "add"; text: "New site"; onClicked: siteDialog.openNew() }
        }
        Text {
            Layout.fillWidth: true
            text: "Any link you add to one of these websites is filed here for you. Pick a site, or add your own."
            color: page.th.subtext
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        // the sites
        Flow {
            Layout.fillWidth: true
            spacing: 8
            Repeater {
                model: page.sitesList
                delegate: Rectangle {
                    id: chip
                    required property var modelData
                    required property int index
                    readonly property bool on: page.sel === index
                    height: 34
                    width: chipRow.implicitWidth + 28
                    radius: 17
                    color: on ? Qt.alpha(modelData.color, 0.25) : Qt.rgba(1, 1, 1, 0.06)
                    border.width: 1
                    border.color: on || cm.containsMouse ? modelData.color : Qt.rgba(1, 1, 1, 0.16)
                    Row {
                        id: chipRow
                        anchors.centerIn: parent
                        spacing: 8
                        SiteIcon { anchors.verticalCenter: parent.verticalCenter; image: chip.modelData.image; name: chip.modelData.icon; size: 16; color: chip.modelData.color }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: chip.modelData.name
                            color: page.th.text
                            font.pixelSize: 13
                            font.weight: chip.on ? Font.DemiBold : Font.Normal
                        }
                        Text {
                            visible: chip.modelData.links > 0
                            anchors.verticalCenter: parent.verticalCenter
                            text: chip.modelData.links
                            color: page.th.subtext
                            font.pixelSize: 12
                        }
                    }
                    MouseArea {
                        id: cm
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: page.sel = chip.index
                    }
                }
            }
        }

        // hosts you keep many links on that are not a site yet
        Repeater {
            model: page.suggestions
            delegate: Banner {
                required property var modelData
                message: "You have " + modelData.count + " links on " + modelData.host + ". Make " + modelData.name + " a site?"
                actionText: "Make it a site"
                onAccepted: core.sites.acceptSuggestion(modelData.host)
                onDismissed: core.sites.dismissSuggestion(modelData.host)
            }
        }
        Banner {
            visible: page.looseN > 0
            message: page.looseN + " link(s) you added earlier belong to your sites but are not filed yet."
            actionText: "Organize"
            showDismiss: false
            onAccepted: core.sites.organizeAll()
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

                // the chosen site: what it covers, and a way to change that
                RowLayout {
                    visible: page.cur !== null
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        Layout.fillWidth: true
                        text: page.cur ? page.cur.hostsText + (page.cur.autoFile ? "" : "  ·  not filed automatically") : ""
                        color: page.th.subtext
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                    AppButton { text: "Edit site"; onClicked: siteDialog.openEdit(page.cur) }
                }

                Repeater {
                    model: page.groups
                    delegate: ColumnLayout {
                        id: grp
                        required property var modelData
                        readonly property var shown: modelData.links.filter(r =>
                            page.filterText === "" || (r.name + " " + r.path).toLowerCase().indexOf(page.filterText) >= 0)
                        visible: shown.length > 0
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text {
                                text: grp.modelData.name.toUpperCase()
                                color: page.cur ? page.cur.color : page.th.accent
                                font.pixelSize: 13
                                font.letterSpacing: 4
                                font.weight: Font.DemiBold
                            }
                            Text {
                                text: grp.shown.length + (grp.shown.length === 1 ? " link" : " links")
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
                                    SiteIcon { x: 16; anchors.verticalCenter: parent.verticalCenter
                                           image: page.cur ? page.cur.image : ""
                                           name: page.cur ? page.cur.icon : "web"; size: page.style === "compact" ? 16 : 22
                                           color: page.cur ? page.cur.color : page.th.accent }
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
                    text: page.cur
                        ? "No " + page.cur.name + " links yet.\nAdd or import a link from " + page.cur.hostsText
                          + " (the browser extension works too) and it appears here by itself."
                        : "No sites yet. Press New site to add one."
                }
                Item { Layout.preferredHeight: 8 }
            }
        }
    }
}
