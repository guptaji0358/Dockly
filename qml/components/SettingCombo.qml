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
    // Optional free-entry value: an option with value -1 ("Custom") shows a number box next to the combo.
    property int customMin: 1
    property int customMax: 86400
    property string customSuffix: ""
    property int customDefault: customMin
    readonly property bool customActive: {
        const i = combo.currentIndex
        return i >= 0 && i < options.length && options[i].value === -1
    }

    // set once "Custom…" is picked, so a custom number equal to a preset doesn't flip the combo back
    property bool customPicked: false

    function customIndex() {
        for (let j = 0; j < options.length; ++j)
            if (options[j].value === -1)
                return j
        return -1
    }

    function sync() {
        const cur = core.settings.values[key]
        if (customPicked && customIndex() >= 0) {
            combo.currentIndex = customIndex()
            return
        }
        for (let i = 0; i < options.length; ++i)
            if (options[i].value === cur) {
                combo.currentIndex = i
                return
            }
        if (customMin > 1 && cur > 0 && cur < customMin) {   // saved before the minimum existed
            core.settings.set(key, customMin)
            return
        }
        for (let j = 0; j < options.length; ++j)
            if (options[j].value === -1) {
                combo.currentIndex = j
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
        onActivated: index => {
            const v = row.options[index].value
            row.customPicked = v === -1
            if (v !== -1)
                core.settings.set(row.key, v)
            else if (core.settings.values[row.key] < row.customMin)
                core.settings.set(row.key, row.customDefault)
            else
                row.sync()
        }

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
    SpinBox {
        id: custom
        visible: row.customActive
        from: row.customMin
        to: row.customMax
        editable: true
        value: Math.max(row.customMin, core.settings.values[row.key])
        onValueModified: core.settings.set(row.key, value)
        textFromValue: (v) => v + row.customSuffix
        valueFromText: (t) => parseInt(t) || row.customMin
        Connections {
            target: core.settings
            function onValuesChanged() { custom.value = Math.max(row.customMin, core.settings.values[row.key]) }
        }
    }
    Item { Layout.fillWidth: true }
}
