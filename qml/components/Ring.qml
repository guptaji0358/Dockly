import QtQuick
import LH

// Radial command ring: sections on the inner ring, favourites on the outer one.
// Open with the orb, Ctrl+Space or the middle mouse button; Esc / click outside closes it.
Item {
    id: ring
    property var tabs: []
    property int current: 0
    property var host
    property bool opened: false
    signal picked(int index)

    readonly property var th: core.theme
    readonly property real p: shown
    property real shown: opened ? 1 : 0
    Behavior on shown {
        enabled: Fx.animated
        NumberAnimation { duration: 340; easing.type: opened ? Easing.OutBack : Easing.InCubic; easing.overshoot: 1.2 }
    }
    property string hint: ""

    visible: shown > 0.01
    focus: opened
    Keys.onEscapePressed: opened = false

    readonly property real cx: width / 2
    readonly property real cy: height / 2
    readonly property real r1: Math.min(width, height) * 0.25
    readonly property real r2: r1 + 112

    ItemModel { id: favs; mode: LH.ModeFavorites }

    function close() { opened = false }

    // backdrop
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(ring.th.bg.r, ring.th.bg.g, ring.th.bg.b, 0.82 * ring.p)
        MouseArea { anchors.fill: parent; onClicked: ring.close() }
    }

    // guide circles
    Repeater {
        model: [ring.r1, ring.r2]
        delegate: Rectangle {
            required property real modelData
            required property int index
            readonly property real rr: modelData * ring.p
            x: ring.cx - rr; y: ring.cy - rr; width: rr * 2; height: rr * 2
            radius: rr
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(ring.th.accent.r, ring.th.accent.g, ring.th.accent.b, index === 0 ? 0.35 : 0.16)
            opacity: ring.p
        }
    }

    // core
    Item {
        x: ring.cx - 64; y: ring.cy - 64; width: 128; height: 128
        scale: 0.5 + 0.5 * ring.p
        opacity: ring.p
        Rectangle {
            anchors.fill: parent
            radius: 64
            color: Qt.rgba(ring.th.accent.r, ring.th.accent.g, ring.th.accent.b, 0.16)
            border.width: 2
            border.color: ring.th.accent
        }
        Icon { anchors.horizontalCenter: parent.horizontalCenter; y: 26; name: "logo"; size: 40; color: ring.th.accent }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 76
            width: 110
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: ring.hint !== "" ? ring.hint : "DOCKLY"
            color: ring.th.text
            font.pixelSize: 12
            font.letterSpacing: 2
            font.weight: Font.DemiBold
        }
    }

    // sections
    Repeater {
        model: ring.tabs
        delegate: Item {
            id: node
            required property int index
            required property var modelData
            readonly property real ang: -Math.PI / 2 + 2 * Math.PI * index / ring.tabs.length
            readonly property bool on: ring.current === index
            width: 76; height: 76
            x: ring.cx + Math.cos(ang) * ring.r1 * ring.p - width / 2
            y: ring.cy + Math.sin(ang) * ring.r1 * ring.p - height / 2
            opacity: Math.min(1, ring.p * 1.4)
            scale: (0.4 + 0.6 * ring.p) * (nm.containsMouse ? 1.14 : 1)
            Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 120 } }

            Rectangle {
                anchors.fill: parent
                radius: 38
                color: node.on ? ring.th.accent : Qt.rgba(ring.th.surface.r, ring.th.surface.g, ring.th.surface.b, 0.92)
                border.width: nm.containsMouse ? 2 : 1
                border.color: nm.containsMouse || node.on ? ring.th.accent : ring.th.border
            }
            Icon {
                anchors.centerIn: parent
                name: node.modelData.icon
                size: 30
                color: node.on ? "white" : ring.th.text
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.bottom
                anchors.topMargin: 6
                text: node.modelData.name
                color: nm.containsMouse || node.on ? ring.th.accent : ring.th.subtext
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            MouseArea {
                id: nm
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onContainsMouseChanged: ring.hint = containsMouse ? node.modelData.full.toUpperCase() : ""
                onClicked: { ring.close(); ring.picked(node.index) }
            }
        }
    }

    // favourites: tap to launch
    Repeater {
        model: favs
        delegate: Item {
            id: fav
            required property int index
            required property string itemId
            required property string name
            required property string iconSource
            readonly property int total: Math.min(favs.count, 14)
            readonly property real ang: -Math.PI / 2 + Math.PI / total + 2 * Math.PI * index / Math.max(1, total)
            visible: index < 14
            width: 54; height: 54
            x: ring.cx + Math.cos(ang) * ring.r2 * ring.p - width / 2
            y: ring.cy + Math.sin(ang) * ring.r2 * ring.p - height / 2
            opacity: Math.min(1, ring.p * 1.4)
            scale: (0.4 + 0.6 * ring.p) * (fm.containsMouse ? 1.2 : 1)
            Behavior on scale { enabled: Fx.animated; NumberAnimation { duration: 120 } }

            Rectangle {
                anchors.fill: parent
                radius: 27
                color: Qt.rgba(ring.th.surface.r, ring.th.surface.g, ring.th.surface.b, 0.92)
                border.width: 1
                border.color: fm.containsMouse ? ring.th.favorite : ring.th.border
            }
            Image {
                anchors.centerIn: parent
                width: 30; height: 30
                source: fav.iconSource
                sourceSize: Qt.size(60, 60)
                fillMode: Image.PreserveAspectFit
                mipmap: true
            }
            MouseArea {
                id: fm
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onContainsMouseChanged: ring.hint = containsMouse ? fav.name.toUpperCase() : ""
                onClicked: { ring.close(); ring.host.launch(fav.itemId) }
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 26
        opacity: ring.p
        text: "Sections  ·  inner ring        Favourites  ·  outer ring        Esc to close"
        color: ring.th.subtext
        font.pixelSize: 12
        font.letterSpacing: 1
    }
}
