import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// "Label    [ combo ]" row bound to a setting key. options: [{text, value}, ...]
RowLayout {
    id: row
    property string key
    property string label
    property var options: []
    readonly property var th: core.theme

    function sync() {
        for (let i = 0; i < options.length; ++i)
            if (options[i].value === core.settings.values[key]) {
                combo.currentIndex = i
                return
            }
        combo.currentIndex = 0
    }
    Component.onCompleted: sync()
    Connections {
        target: core.settings
        function onValuesChanged() { row.sync() }
    }

    spacing: 16
    Text {
        text: row.label
        color: row.th.text
        Layout.preferredWidth: 170
    }
    ComboBox {
        id: combo
        Layout.preferredWidth: 240
        implicitHeight: 38
        model: row.options
        textRole: "text"
        onActivated: index => core.settings.set(row.key, row.options[index].value)

        contentItem: Text {
            text: combo.displayText
            color: row.th.text
            verticalAlignment: Text.AlignVCenter
            leftPadding: 12
        }
        background: Rectangle {
            radius: 8
            color: row.th.bg
            border.color: combo.activeFocus || combo.hovered ? row.th.accent : row.th.border
        }
        indicator: Text {
            x: combo.width - width - 12
            y: (combo.height - height) / 2
            text: "▾"
            color: row.th.subtext
        }
        delegate: ItemDelegate {
            id: opt
            required property int index
            required property var modelData
            width: combo.width
            contentItem: Text {
                text: opt.modelData.text
                color: opt.highlighted ? "white" : row.th.text
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { color: opt.highlighted ? row.th.accent : row.th.surface }
            highlighted: combo.highlightedIndex === index
        }
        popup: Popup {
            y: combo.height + 2
            width: combo.width
            implicitHeight: Math.min(contentItem.implicitHeight + 2, 260)
            padding: 1
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
            }
            background: Rectangle { color: row.th.surface; border.color: row.th.border; radius: 8 }
        }
    }
    Item { Layout.fillWidth: true }
}
