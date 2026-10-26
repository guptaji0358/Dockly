import QtQuick
import QtQuick.Controls

// Borderless text field with a glowing underline instead of a boxed Windows-style edit.
TextField {
    id: f
    readonly property var th: core.theme
    implicitHeight: 36
    color: th.text
    placeholderTextColor: th.subtext
    selectByMouse: true
    verticalAlignment: TextInput.AlignVCenter
    leftPadding: 2
    rightPadding: 2
    background: Item {
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Qt.rgba(1, 1, 1, 0.18)
        }
        Rectangle {   // accent underline grows from the left on focus
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            height: 2
            width: f.activeFocus ? parent.width : 0
            color: f.th.accent
            Behavior on width { enabled: Fx.animated; NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
        }
    }
}
