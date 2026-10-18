import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Software browser: programs from the hidden catalog as glowing tiles. Tap tiles to load them
// into the cargo tray, then add the whole selection. A radar sweep shows while scanning.
AppDialog {
    id: dlg
    property var entries: []
    property var picked: ({})   // target -> name
    readonly property int pickedCount: Object.keys(picked).length
    readonly property int total: core.catalogCount

    title: ""
    padding: 0
    width: Math.min(900, (Overlay.overlay ? Overlay.overlay.width : 900) - 60)

    function openPicker() {
        picked = ({})
        search.text = ""
        reload()
        open()
    }
    function reload() { entries = core.catalogSearch(search.text, 300) }
    function toggle(target, name) {
        const p = Object.assign({}, picked)
        if (p[target] !== undefined)
            delete p[target]
        else
            p[target] = name
        picked = p
    }

    Connections {
        target: core
        function onCatalogChanged() { dlg.reload() }
    }

    // ---- header: radar + search + actions ----------------------------------------
    RowLayout {
        Layout.fillWidth: true
        Layout.margins: 22
        Layout.bottomMargin: 6
        spacing: 16

        Item {   // radar
            Layout.preferredWidth: 56
            Layout.preferredHeight: 56
            Rectangle {
                anchors.fill: parent; radius: 28
                color: Qt.rgba(dlg.th.accent.r, dlg.th.accent.g, dlg.th.accent.b, 0.08)
                border.width: 2; border.color: dlg.th.accent
            }
            Rectangle { anchors.centerIn: parent; width: 34; height: 34; radius: 17; color: "transparent"
                        border.width: 1; border.color: Qt.rgba(dlg.th.accent.r, dlg.th.accent.g, dlg.th.accent.b, 0.4) }
            Rectangle { anchors.centerIn: parent; width: 8; height: 8; radius: 4; color: dlg.th.accent }
            Item {   // sweeping beam
                anchors.fill: parent
                visible: core.scanning
                RotationAnimator on rotation { from: 0; to: 360; duration: 1600; loops: Animation.Infinite; running: core.scanning }
                Rectangle {
                    x: parent.width / 2; y: 3; width: 2; height: parent.height / 2 - 3
                    gradient: Gradient {
                        GradientStop { position: 0; color: dlg.th.accent }
                        GradientStop { position: 1; color: "transparent" }
                    }
                }
            }
        }
        ColumnLayout {
            spacing: 0
            Text {
                text: "SOFTWARE BROWSER"
                color: dlg.th.text
                font.pixelSize: 17
                font.weight: Font.Bold
                font.letterSpacing: 4
            }
            Text {
                text: core.scanning ? "Scanning your Start Menu…"
                      : dlg.total === 0 ? "Nothing detected yet - scan this PC or browse for an .exe"
                      : dlg.total + " programs detected" + (dlg.entries.length >= 300 ? "  ·  showing 300, search to narrow" : "")
                color: dlg.th.subtext
                font.pixelSize: 12
            }
        }
        Item { Layout.fillWidth: true }
        Glass {
            Layout.preferredWidth: 260
            Layout.preferredHeight: 40
            radius: 20
            tint: 0.5
            border.color: search.activeFocus ? dlg.th.accent : edge
            Icon { x: 14; anchors.verticalCenter: parent.verticalCenter; name: "search"; size: 16; color: dlg.th.subtext }
            TextField {
                id: search
                anchors.fill: parent
                leftPadding: 40
                rightPadding: 12
                placeholderText: "Filter programs…"
                color: dlg.th.text
                placeholderTextColor: dlg.th.subtext
                verticalAlignment: TextInput.AlignVCenter
                selectByMouse: true
                background: null
                onTextChanged: dlg.reload()
            }
        }
        OrbButton {
            iconName: "scan"
            tip: core.scanning ? "Scanning…" : "Scan this PC"
            enabled: !core.scanning
            onClicked: core.refreshSoftware()
        }
        OrbButton {
            iconName: "open"
            tip: "Browse for an .exe…"
            onClicked: {
                const f = core.pickFiles("Add Software", core.fileFilter(0), true)
                if (f.length > 0) {
                    core.addFiles(0, f)
                    dlg.close()
                }
            }
        }
        OrbButton { diameter: 34; iconName: "close"; tip: "Close"; onClicked: dlg.close() }
    }

    // ---- tile grid ---------------------------------------------------------------------
    GridView {
        id: grid
        Layout.fillWidth: true
        Layout.preferredHeight: 370
        Layout.leftMargin: 16
        Layout.rightMargin: 10
        clip: true
        model: dlg.entries
        cellWidth: Math.floor((width - 10) / Math.max(3, Math.floor((width - 10) / 140)))
        cellHeight: 132
        ScrollBar.vertical: ScrollBar {}

        delegate: Item {
            id: cell
            required property var modelData
            required property int index
            readonly property bool on: dlg.picked[modelData.target] !== undefined
            width: grid.cellWidth
            height: grid.cellHeight

            Rectangle {
                id: tile
                anchors.fill: parent
                anchors.margins: 6
                radius: 20
                scale: tm.pressed ? 0.96 : (tm.containsMouse ? 1.04 : 1)
                Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 110 } }
                color: cell.on ? Qt.rgba(dlg.th.accent.r, dlg.th.accent.g, dlg.th.accent.b, 0.22)
                              : Qt.rgba(1, 1, 1, tm.containsMouse ? 0.09 : 0.045)
                border.width: cell.on ? 2 : 1
                border.color: cell.on ? dlg.th.accent : Qt.rgba(1, 1, 1, tm.containsMouse ? 0.3 : 0.12)
                Behavior on color { enabled: Fx.animated; ColorAnimation { duration: 120 } }

                Rectangle {   // letter tile while (or if) the shell has no icon
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 16
                    width: 46; height: 46; radius: 14
                    visible: appIcon.status !== Image.Ready
                    color: Qt.rgba(dlg.th.accent.r, dlg.th.accent.g, dlg.th.accent.b, 0.25)
                    Text { anchors.centerIn: parent; text: cell.modelData.name.charAt(0).toUpperCase()
                           color: dlg.th.accent; font.pixelSize: 22; font.weight: Font.Bold }
                }
                Image {
                    id: appIcon
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 16
                    width: 46; height: 46
                    source: "image://item/path:" + encodeURIComponent(cell.modelData.target)
                    sourceSize: Qt.size(92, 92)
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    mipmap: true
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 72
                    width: parent.width - 16
                    text: cell.modelData.name
                    color: dlg.th.text
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                Rectangle {   // check badge
                    visible: cell.on
                    x: parent.width - 26; y: 6
                    width: 20; height: 20; radius: 10
                    color: dlg.th.accent
                    Icon { anchors.centerIn: parent; name: "check"; size: 12; color: "white" }
                }
                MouseArea {
                    id: tm
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dlg.toggle(cell.modelData.target, cell.modelData.name)
                }
                ToolTip.visible: tm.containsMouse
                ToolTip.delay: 700
                ToolTip.text: cell.modelData.target
            }
        }

        Text {
            anchors.centerIn: parent
            visible: grid.count === 0
            text: core.scanning ? "Scanning…" : "No programs to show"
            color: dlg.th.subtext
        }
    }

    // ---- cargo tray -----------------------------------------------------------------------
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 74
        color: Qt.rgba(0, 0, 0, 0.22)
        bottomLeftRadius: 22
        bottomRightRadius: 22

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 22
            anchors.rightMargin: 22
            spacing: 14

            Text {
                text: dlg.pickedCount === 0 ? "TAP TILES TO SELECT" : dlg.pickedCount + " SELECTED"
                color: dlg.pickedCount === 0 ? dlg.th.subtext : dlg.th.accent
                font.pixelSize: 11
                font.letterSpacing: 3
                font.weight: Font.DemiBold
            }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                orientation: ListView.Horizontal
                spacing: 6
                clip: true
                model: Object.keys(dlg.picked)
                delegate: Rectangle {
                    required property string modelData
                    width: 38; height: 38; radius: 19
                    color: Qt.rgba(dlg.th.accent.r, dlg.th.accent.g, dlg.th.accent.b, 0.18)
                    border.width: 1; border.color: dlg.th.accent
                    Image {
                        anchors.centerIn: parent
                        width: 22; height: 22
                        source: "image://item/path:" + encodeURIComponent(parent.modelData)
                        sourceSize: Qt.size(44, 44)
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: dlg.toggle(parent.modelData, "") }
                }
                add: Transition { NumberAnimation { property: "scale"; from: 0; to: 1; duration: 200; easing.type: Easing.OutBack } }
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
                    if (list.length > 0)
                        core.addSoftware(list)
                    dlg.close()
                }
            }
        }
    }
}
