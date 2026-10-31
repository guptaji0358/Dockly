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

    readonly property rect area: workArea

    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    title: "Dockly quick launcher"
    visible: false
    width: 640
    height: panel.height
    x: area.x + (area.width - width) / 2
    y: area.y + area.height * 0.2

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
        else if (armed && visible)
            hide()
    }
    onVisibleChanged: if (!visible) armed = false

    Connections {
        target: core
        enabled: ql.visible
        function onItemsChanged() { ql.refresh() }
    }

    Rectangle {
        id: panel
        width: parent.width
        height: bar.height + (list.count > 0 || field.text !== "" ? list.height + hint.height + 18 : 0)
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
            TextField {
                id: field
                anchors.fill: parent
                leftPadding: 60
                rightPadding: 22
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
                        ql.hide()
                        event.accepted = true
                    }
                }
            }
        }
        Rectangle {
            visible: list.count > 0
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
            visible: list.count > 0 || field.text !== ""
            text: field.text === "" ? "Recent and favourites   ·   type to search" : "↑↓ select   ·   Enter open   ·   Ctrl+Enter show in folder   ·   Esc close"
            color: ql.th.subtext
            font.pixelSize: 11
        }
    }
}
