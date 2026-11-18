import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Message + a row of buttons. ask(title, text, labels, callback(index), dangerIndex).
AppDialog {
    id: dlg
    property string text
    property var labels: []
    property int dangerIndex: -1
    property var callback: null

    function ask(title, text, labels, callback, dangerIndex) {
        dlg.title = title
        dlg.text = text
        dlg.labels = labels
        dlg.callback = callback || null
        dlg.dangerIndex = dangerIndex === undefined ? -1 : dangerIndex
        open()
    }

    Text {
        Layout.fillWidth: true
        text: dlg.text
        color: dlg.th.subtext
        wrapMode: Text.Wrap
        visible: text !== ""
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 6
        spacing: 8
        Item { Layout.fillWidth: true }
        Repeater {
            model: dlg.labels
            AppButton {
                required property int index
                required property var modelData
                text: modelData
                primary: index === 0 && dlg.dangerIndex !== 0 && dlg.labels.length > 1
                danger: index === dlg.dangerIndex
                onClicked: {
                    dlg.close()
                    if (dlg.callback)
                        dlg.callback(index)
                }
            }
        }
    }
}
