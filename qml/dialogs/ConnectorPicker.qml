import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Pick which imported entries (bookmarks, repos, projects) become web shortcuts.
AppDialog {
    id: dlg
    property string heading: ""
    property string note: ""
    property var entries: []
    property var picked: ({})        // target -> name
    readonly property int pickedCount: Object.keys(picked).length
    readonly property var shown: {
        const q = filter.text.trim().toLowerCase()
        return q === "" ? entries : entries.filter(e => (e.name + " " + e.sub + " " + e.target).toLowerCase().indexOf(q) >= 0)
    }

    title: ""
    padding: 0
    width: Math.min(760, (Overlay.overlay ? Overlay.overlay.width : 760) - 60)

    property bool groupable: false   // offer "put these in a new category" (used for GitHub repos)

    function openWith(heading, entries, note, groupable) {
        dlg.heading = heading
        dlg.entries = entries
        dlg.note = note || ""
        dlg.groupable = groupable === true
        picked = ({})
        filter.text = ""
        groupName.text = ""
        open()
    }
    function toggle(target, name) {
        const p = Object.assign({}, picked)
        if (p[target] !== undefined)
            delete p[target]
        else
            p[target] = name
        picked = p
    }
    function selectShown() {
        const p = Object.assign({}, picked)
        for (const e of shown)
            p[e.target] = e.name
        picked = p
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: 22
        Layout.bottomMargin: 8
        spacing: 14
        ColumnLayout {
            spacing: 0
            Text {
                text: dlg.heading.toUpperCase()
                color: dlg.th.text
                font.pixelSize: 16
                font.weight: Font.Bold
                font.letterSpacing: 4
            }
            Text {
                text: dlg.entries.length + " found" + (dlg.note !== "" ? "  ·  " + dlg.note : "")
                color: dlg.th.subtext
                font.pixelSize: 12
            }
        }
        Item { Layout.fillWidth: true }
        Glass {
            Layout.preferredWidth: 230
            Layout.preferredHeight: 38
            radius: 19
            tint: 0.5
            border.color: filter.activeFocus ? dlg.th.accent : edge
            Icon { x: 13; anchors.verticalCenter: parent.verticalCenter; name: "search"; size: 15; color: dlg.th.subtext }
            TextField {
                id: filter
                anchors.fill: parent
                leftPadding: 38
                rightPadding: 12
                placeholderText: "Filter…"
                color: dlg.th.text
                placeholderTextColor: dlg.th.subtext
                verticalAlignment: TextInput.AlignVCenter
                selectByMouse: true
                background: null
            }
        }
        OrbButton { diameter: 34; iconName: "check"; tip: "Select all shown"; onClicked: dlg.selectShown() }
        OrbButton { diameter: 34; iconName: "close"; tip: "Close"; onClicked: dlg.close() }
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.preferredHeight: 360
        Layout.leftMargin: 14
        Layout.rightMargin: 8
        clip: true
        spacing: 4
        model: dlg.shown
        ScrollBar.vertical: ScrollBar {}

        delegate: Rectangle {
            id: row
            required property var modelData
            readonly property bool on: dlg.picked[modelData.target] !== undefined
            width: ListView.view.width - 10
            height: 52
            radius: 14
            color: on ? Qt.rgba(dlg.th.accent.r, dlg.th.accent.g, dlg.th.accent.b, 0.2)
                      : Qt.rgba(1, 1, 1, rm.containsMouse ? 0.08 : 0.04)
            border.width: on ? 1.5 : 1
            border.color: on ? dlg.th.accent : Qt.rgba(1, 1, 1, 0.1)

            Rectangle {
                id: dot
                x: 14; anchors.verticalCenter: parent.verticalCenter
                width: 22; height: 22; radius: 11
                color: row.on ? dlg.th.accent : "transparent"
                border.width: 2
                border.color: row.on ? dlg.th.accent : Qt.rgba(1, 1, 1, 0.3)
                Icon { anchors.centerIn: parent; visible: row.on; name: "check"; size: 13; color: "white" }
            }
            Column {
                anchors.left: dot.right
                anchors.leftMargin: 14
                anchors.right: parent.right
                anchors.rightMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    width: parent.width
                    text: row.modelData.name
                    color: dlg.th.text
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: row.modelData.sub
                    color: dlg.th.subtext
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                }
            }
            MouseArea {
                id: rm
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: dlg.toggle(row.modelData.target, row.modelData.name)
            }
        }
        Text {
            anchors.centerIn: parent
            visible: list.count === 0
            text: "Nothing to show"
            color: dlg.th.subtext
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: dlg.groupable ? 130 : 70
        color: Qt.rgba(0, 0, 0, 0.22)
        bottomLeftRadius: 22
        bottomRightRadius: 22

        // GitHub: keep the picks together in their own category
        Column {
            visible: dlg.groupable
            x: 22; y: 10
            width: parent.width - 44
            spacing: 6
            HudField {
                id: groupName
                width: parent.width
                placeholderText: "New category name, e.g. Work repos (leave empty to add them loose)"
            }
            Text {
                width: parent.width
                text: "Tip: pick only your unique repos. Forks and old copies make the list long again. "
                    + "Using a name you already made adds these repos to that category."
                color: dlg.th.subtext
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }
        }
        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 70
            anchors.leftMargin: 22
            anchors.rightMargin: 22
            spacing: 12
            Text {
                Layout.fillWidth: true
                text: dlg.pickedCount === 0 ? "TAP ROWS TO SELECT" : dlg.pickedCount + " SELECTED"
                color: dlg.pickedCount === 0 ? dlg.th.subtext : dlg.th.accent
                font.pixelSize: 11
                font.letterSpacing: 3
                font.weight: Font.DemiBold
            }
            AppButton { text: "Cancel"; onClicked: dlg.close() }
            AppButton {
                text: dlg.pickedCount > 0 ? "Add " + dlg.pickedCount : "Add Selected"
                primary: true
                enabled: dlg.pickedCount > 0
                onClicked: {
                    const list = []
                    for (const t in dlg.picked)
                        list.push({target: t, name: dlg.picked[t]})
                    if (dlg.groupable && groupName.text.trim() !== "")
                        core.connectors.addEntriesToNewCollection(list, groupName.text)
                    else
                        core.connectors.addEntries(list)
                    dlg.close()
                }
            }
        }
    }
}
