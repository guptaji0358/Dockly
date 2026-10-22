import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import LH

// Inspector for the selected card: a floating HUD glass card with a hexagon hero, an underline
// editor, a glowing OPEN pill and round action buttons.
Glass {
    id: panel
    property var host
    property var info: ({})
    property bool dirty: false
    readonly property bool collection: info.category === LH.Collection
    readonly property bool web: info.category === LH.Web
    readonly property color accent: info.category === undefined ? th.accent : th.accentOf(info.category)

    readonly property bool shown: info.name !== undefined
    implicitWidth: 300
    radius: 22
    tint: 0.9
    brackets: true
    edge: Qt.rgba(accent.r, accent.g, accent.b, 0.4)

    function load() {
        const i = core.selectedId === "" ? ({}) : core.itemInfo(core.selectedId)
        info = i
        dirty = false
        if (i.name !== undefined) {
            nameField.text = i.name
            targetField.text = i.target
        }
    }
    function refresh() {
        if (core.selectedId === "")
            return
        const i = core.itemInfo(core.selectedId)
        if (i.name === undefined) {
            core.select("")
            return
        }
        info = i
        if (!dirty) {
            nameField.text = i.name
            targetField.text = i.target
        }
    }
    Component.onCompleted: load()
    Connections {
        target: core
        function onSelectionChanged() { panel.load() }
        function onItemsChanged() { panel.refresh() }
    }

    component Label: Text {
        color: panel.accent
        font.pixelSize: 10
        font.letterSpacing: 3
        font.weight: Font.DemiBold
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 6

        // hero
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 104

            Item {
                id: hero
                width: 90; height: 104
                Shape {
                    anchors.fill: parent
                    preferredRendererType: Shape.CurveRenderer
                    ShapePath {
                        strokeWidth: 2
                        strokeColor: panel.accent
                        joinStyle: ShapePath.RoundJoin
                        fillGradient: LinearGradient {
                            y1: 0; y2: 104
                            GradientStop { position: 0; color: Qt.rgba(panel.accent.r, panel.accent.g, panel.accent.b, 0.35) }
                            GradientStop { position: 1; color: Qt.rgba(panel.th.bg.r, panel.th.bg.g, panel.th.bg.b, 0.7) }
                        }
                        startX: 45; startY: 2
                        PathLine { x: 88; y: 27 }
                        PathLine { x: 88; y: 77 }
                        PathLine { x: 45; y: 102 }
                        PathLine { x: 2; y: 77 }
                        PathLine { x: 2; y: 27 }
                        PathLine { x: 45; y: 2 }
                    }
                }
                Image {
                    anchors.centerIn: parent
                    width: 48; height: 48
                    source: panel.info.id === undefined ? "" : "image://item/" + panel.info.id + "?v=" + core.iconVersion
                    sourceSize: Qt.size(128, 128)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }
            Column {
                anchors.left: hero.right
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Label { text: (panel.info.categoryName || "").toUpperCase() }
                Text {
                    visible: panel.collection
                    text: panel.info.memberCount + " items"
                    color: panel.th.subtext
                    font.pixelSize: 13
                }
            }
            OrbButton {
                anchors.right: parent.right
                anchors.top: parent.top
                diameter: 30
                iconName: "close"
                tip: "Close"
                onClicked: core.select("")
            }
        }

        Label { text: "NAME"; Layout.topMargin: 6 }
        HudField {
            id: nameField
            Layout.fillWidth: true
            font.pixelSize: 18
            font.weight: Font.DemiBold
            onTextEdited: panel.dirty = true
        }

        Label { text: panel.web ? "URL" : "PATH"; visible: !panel.collection; Layout.topMargin: 8 }
        RowLayout {
            Layout.fillWidth: true
            visible: !panel.collection
            spacing: 6
            HudField {
                id: targetField
                Layout.fillWidth: true
                font.pixelSize: 13
                onTextEdited: panel.dirty = true
            }
            OrbButton {
                visible: !panel.web
                diameter: 30
                iconName: "locate"
                tip: "Browse…"
                onClicked: {
                    const f = core.pickFiles("Choose file", core.fileFilter(panel.info.category), false)
                    if (f.length > 0) {
                        targetField.text = f[0]
                        panel.dirty = true
                    }
                }
            }
        }

        // status pill
        Rectangle {
            visible: !panel.collection
            Layout.topMargin: 10
            implicitHeight: 26
            implicitWidth: statusText.implicitWidth + 34
            radius: 13
            readonly property color tone: panel.info.status === 2 ? panel.th.missing
                                        : panel.info.status === 1 ? panel.th.warning : "#3ddc97"
            color: Qt.rgba(tone.r, tone.g, tone.b, 0.14)
            border.width: 1
            border.color: Qt.rgba(tone.r, tone.g, tone.b, 0.5)
            Rectangle { x: 11; anchors.verticalCenter: parent.verticalCenter; width: 8; height: 8; radius: 4; color: parent.tone }
            Text {
                id: statusText
                x: 26
                anchors.verticalCenter: parent.verticalCenter
                color: parent.tone
                font.pixelSize: 12
                text: panel.info.status === 2 ? "Missing at this path"
                    : panel.info.status === 1 ? "Link looks invalid" : "Ready"
            }
        }

        Item { Layout.fillHeight: true }

        // OPEN pill
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Rectangle {   // glow
                anchors.fill: parent
                anchors.margins: -5
                radius: height / 2
                color: panel.accent
                opacity: openMouse.containsMouse ? 0.3 : 0.14
                visible: Fx.glow
                Behavior on opacity { enabled: Fx.animated; NumberAnimation { duration: 140 } }
            }
            Rectangle {
                anchors.fill: parent
                radius: height / 2
                scale: openMouse.pressed ? 0.97 : 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: Qt.lighter(panel.accent, 1.25) }
                    GradientStop { position: 1; color: Qt.darker(panel.accent, 1.25) }
                }
                Row {
                    anchors.centerIn: parent
                    spacing: 10
                    Icon { name: "open"; size: 20; color: "white"; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: panel.collection ? "LAUNCH ALL" : "OPEN"
                        color: "white"
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        font.letterSpacing: 3
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            MouseArea {
                id: openMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: panel.host.launch(panel.info.id)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            spacing: 10
            Item { Layout.fillWidth: true }
            OrbButton {
                iconName: panel.info.favorite ? "star-fill" : "star"
                tip: panel.info.favorite ? "Remove from favourites" : "Add to favourites"
                onClicked: core.setFavorite(panel.info.id, !panel.info.favorite)
            }
            OrbButton {
                iconName: "check"
                tip: "Save changes"
                enabled: panel.dirty
                primary: panel.dirty
                onClicked: {
                    if (core.updateItem(panel.info.id, nameField.text, targetField.text))
                        panel.load()
                }
            }
            OrbButton {
                visible: panel.info.status !== 0 && !panel.web && !panel.collection
                iconName: "locate"
                tip: "Locate again…"
                onClicked: {
                    const f = core.pickFiles("Locate " + nameField.text, core.fileFilter(panel.info.category), false)
                    if (f.length > 0) {
                        core.updateItem(panel.info.id, nameField.text, f[0])
                        panel.load()
                    }
                }
            }
            OrbButton {
                iconName: "edit"
                tip: "Change icon / edit…"
                onClicked: panel.host.edit(panel.info.id)
            }
            OrbButton {
                danger: true
                iconName: "trash"
                tip: "Delete card"
                onClicked: panel.host.remove(panel.info.id)
            }
            Item { Layout.fillWidth: true }
        }
    }
}
