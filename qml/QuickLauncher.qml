import QtQuick
import QtQuick.Controls
import QtQuick.Window
import LH

// Type-to-launch box, opened from anywhere by a system-wide shortcut (setting launcherHotkey).
// Searches every item (apps, documents, media, links) and workspaces (collections); Enter opens the match.
Window {
    id: ql

    readonly property var th: core.theme
    readonly property int rowH: 52
    readonly property int maxRows: 8
    property var results: []
    property int current: 0
    property bool armed: false // set once the window really has focus, so losing it later closes the box

    readonly property bool expanded: field.text !== ""
    readonly property rect area: workArea

    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    title: "Dockly quick launcher"
    visible: false
    width: 640
    height: panel.height
    // where the box opens: the spot the user dragged it to (fractions of the work area), else top centre
    readonly property var savedPos: String(core.settings.values.launcherPos).split(",").map(Number)
    readonly property bool hasPos: savedPos.length === 2 && !isNaN(savedPos[0]) && !isNaN(savedPos[1])
    readonly property real fx: hasPos ? savedPos[0] : 0.5
    readonly property real fy: hasPos ? savedPos[1] : 0.2
    x: Math.max(area.x, Math.min(area.x + area.width * fx - width / 2, area.x + area.width - width))
    y: Math.max(area.y, Math.min(area.y + area.height * fy, area.y + area.height - height))

    // drag-to-place: while the box is dragged it is swapped for a ghost over a snapshot of the desktop
    property bool placing: false
    property point pressAt // cursor (screen) when the drag began
    property point startAt // box top-left (screen) when the drag began
    property real ghostX: 0
    property real ghostY: 0
    property bool snapped: false
    property url ghostSrc
    property var ghostGrab
    property string shotSrc
    property int shotN: 0

    function toggle() {
        if (visible)
            hide()
        else
            openWith("")
    }
    function openWith(text) {
        field.text = text
        refresh()
        armed = false
        show()
        raise()
        requestActivate()
        field.forceActiveFocus()
    }
    function dragMove(g) {
        const cx = area.x + (area.width - width) / 2
        let nx = startAt.x + g.x - pressAt.x
        const ny = startAt.y + g.y - pressAt.y
        snapped = Math.abs(nx - cx) < 14 // magnet to the horizontal centre
        if (snapped)
            nx = cx
        ghostX = Math.max(area.x, Math.min(nx, area.x + area.width - width))
        ghostY = Math.max(area.y, Math.min(ny, area.y + area.height - height))
    }
    function beginPlacing(g) {
        placing = true
        shotSrc = ""
        ghostSrc = ""
        panel.grabToImage(r => { ghostGrab = r; ghostSrc = r.url })
        opacity = 0 // keeps receiving the held mouse, but is out of the snapshot
        shotTimer.restart()
        dragMove(g)
    }
    function endPlacing(commit) {
        shotTimer.stop()
        placer.hide()
        placing = false
        opacity = 1
        if (commit) {
            const px = (ghostX + width / 2 - area.x) / area.width
            const py = (ghostY - area.y) / area.height
            core.settings.set("launcherPos", px.toFixed(4) + "," + py.toFixed(4))
        }
        requestActivate()
        field.forceActiveFocus()
    }
    function refresh() {
        results = core.quickSearch(field.text, maxRows)
        current = 0
    }
    function activate(i) {
        if (i < 0 || i >= results.length)
            return
        const id = results[i].id
        hide()
        if (core.launch(id) === LH.TargetMissing)
            tray.openMain(id) // gone from disk: the main window offers the recovery dialog
    }

    onActiveChanged: {
        if (active)
            armed = true
        else if (armed && visible && !placing)
            hide()
    }
    onVisibleChanged: if (!visible) {
        armed = false
        if (placing)
            endPlacing(false)
    }

    Timer {
        id: shotTimer
        interval: 100 // let the box fade out of the compositor before the desktop is photographed
        onTriggered: {
            ql.shotN++
            ql.shotSrc = "image://screen/" + ql.shotN
            placer.show()
        }
    }

    // press-and-drag on any bare part of the bar or hint moves the box; double-click puts it back top-centre
    component DragZone: MouseArea {
        cursorShape: ql.placing ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        acceptedButtons: Qt.LeftButton
        property point downAt
        function globalAt(m) { return mapToGlobal(m.x, m.y) }
        onPressed: m => {
            downAt = globalAt(m)
            ql.pressAt = downAt
            ql.startAt = Qt.point(ql.x, ql.y)
        }
        onPositionChanged: m => {
            if (!pressed)
                return
            const g = globalAt(m)
            if (ql.placing)
                ql.dragMove(g)
            else if (Math.abs(g.x - downAt.x) + Math.abs(g.y - downAt.y) > 8)
                ql.beginPlacing(g)
        }
        onReleased: if (ql.placing) ql.endPlacing(true)
        onCanceled: if (ql.placing) ql.endPlacing(false)
        onDoubleClicked: core.settings.set("launcherPos", "")
    }

    // full-screen stand-in shown while dragging: the real desktop, dimmed, with the box riding on top
    Window {
        id: placer
        flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput | Qt.WindowDoesNotAcceptFocus
        color: "black"
        visible: false
        x: screenRect.x
        y: screenRect.y
        width: screenRect.width
        height: screenRect.height

        Image {
            anchors.fill: parent
            source: ql.shotSrc
            cache: false
            fillMode: Image.Stretch
        }
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.35)
        }
        // vertical guide that appears when the box magnets to the centre
        Rectangle {
            visible: ql.snapped
            x: ql.area.x - screenRect.x + ql.area.width / 2
            width: 1
            height: parent.height
            color: Qt.alpha(ql.th.accent, 0.8)
        }
        Rectangle {
            x: ql.ghostX - screenRect.x
            y: ql.ghostY - screenRect.y
            width: ql.width
            height: ql.height
            radius: ql.th.radius === 0 ? 0 : 18
            color: Qt.alpha(ql.th.surface, 0.97)
            border.width: 2
            border.color: ql.th.accent
            Image {
                anchors.fill: parent
                source: ql.ghostSrc
                fillMode: Image.Stretch
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 48
            text: "Release to open the search box here   \u00b7   Esc to cancel"
            color: "white"
            font.pixelSize: 15
        }
    }

    Connections {
        target: core
        enabled: ql.visible
        function onItemsChanged() { ql.refresh() }
    }

    Rectangle {
        id: panel
        width: parent.width
        // just the input until something is typed, then grows to fit the results
        height: bar.height + (ql.expanded ? list.height + hint.height + 18 : 0)
        radius: th.radius === 0 ? 0 : 18
        color: Qt.alpha(ql.th.surface, 0.97)
        border.width: 1
        border.color: Qt.rgba(ql.th.accent.r, ql.th.accent.g, ql.th.accent.b, 0.5)
        clip: true

        // search field
        Item {
            id: bar
            width: parent.width
            height: 62
            Icon {
                x: 22
                anchors.verticalCenter: parent.verticalCenter
                name: "search"
                size: 22
                color: ql.th.accent
            }
            DragZone { anchors.fill: parent }
            TextField {
                id: field
                anchors.fill: parent
                anchors.leftMargin: 52
                anchors.rightMargin: 12
                anchors.topMargin: 9
                anchors.bottomMargin: 9
                leftPadding: 8
                rightPadding: 10
                placeholderText: "Open an app, file, site or workspace…"
                placeholderTextColor: ql.th.subtext
                color: ql.th.text
                font.pixelSize: 20
                selectByMouse: true
                verticalAlignment: TextInput.AlignVCenter
                background: null
                onTextChanged: ql.refresh()
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Down) {
                        ql.current = Math.min(ql.current + 1, ql.results.length - 1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Up) {
                        ql.current = Math.max(ql.current - 1, 0)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        if ((event.modifiers & Qt.ControlModifier) && ql.results.length > 0) {
                            const id = ql.results[ql.current].id
                            ql.hide()
                            core.showInFolder(id)
                        } else {
                            ql.activate(ql.current)
                        }
                        event.accepted = true
                    } else if (event.key === Qt.Key_Escape) {
                        if (ql.placing)
                            ql.endPlacing(false)
                        else
                            ql.hide()
                        event.accepted = true
                    }
                }
            }
        }
        Rectangle {
            visible: ql.expanded
            anchors.top: bar.bottom
            width: parent.width
            height: 1
            color: Qt.rgba(1, 1, 1, ql.th.dark ? 0.08 : 0.0)
        }

        ListView {
            id: list
            anchors.top: bar.bottom
            anchors.topMargin: 6
            width: parent.width
            height: Math.max(count, 1) * ql.rowH
            interactive: false
            model: ql.results
            currentIndex: ql.current
            delegate: Item {
                id: row
                required property int index
                required property var modelData
                readonly property bool on: ql.current === index
                width: list.width
                height: ql.rowH

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    radius: ql.th.radius === 0 ? 0 : 12
                    color: row.on ? Qt.rgba(ql.th.accent.r, ql.th.accent.g, ql.th.accent.b, ql.th.dark ? 0.22 : 0.16) : "transparent"
                }
                Image {
                    x: 22
                    anchors.verticalCenter: parent.verticalCenter
                    width: 32
                    height: 32
                    source: row.modelData.iconSource
                    sourceSize: Qt.size(64, 64)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 68
                    anchors.right: tag.left
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: row.modelData.name
                        color: ql.th.text
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        visible: text !== ""
                        text: row.modelData.subtitle
                        color: ql.th.subtext
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
                Rectangle {
                    id: tag
                    anchors.right: parent.right
                    anchors.rightMargin: 24
                    anchors.verticalCenter: parent.verticalCenter
                    width: tagText.implicitWidth + 16
                    height: 22
                    radius: ql.th.radius === 0 ? 0 : 11
                    color: Qt.rgba(1, 1, 1, ql.th.dark ? 0.07 : 0.0)
                    border.width: ql.th.dark ? 0 : 1
                    border.color: ql.th.border
                    Text {
                        id: tagText
                        anchors.centerIn: parent
                        text: row.modelData.categoryName
                        color: ql.th.categoryColor(row.modelData.category)
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: ql.current = row.index
                    onClicked: ql.activate(row.index)
                }
            }

            Text {
                visible: list.count === 0 && field.text !== ""
                anchors.centerIn: parent
                text: "No matches"
                color: ql.th.subtext
                font.pixelSize: 14
            }
        }

        Text {
            id: hint
            anchors.top: list.bottom
            anchors.topMargin: 6
            anchors.horizontalCenter: parent.horizontalCenter
            height: 24
            visible: ql.expanded
            text: "↑↓ select   ·   Enter open   ·   Ctrl+Enter show in folder   ·   Esc close"
            color: ql.th.subtext
            font.pixelSize: 11
        }
        DragZone {
            visible: ql.expanded
            anchors.top: list.bottom
            anchors.bottom: parent.bottom
            width: parent.width
        }
    }
}
