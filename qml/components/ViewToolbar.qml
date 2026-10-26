import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Grid / List / Compact switch + card-size slider, shown on every page.
Rectangle {
    id: bar
    readonly property var th: core.theme
    readonly property var styles: ["grid", "list", "compact"]
    readonly property var sizes: ["small", "medium", "large"]

    implicitWidth: row.implicitWidth + 18
    implicitHeight: 46
    radius: th.radius === 0 ? 0 : 12
    color: th.surface
    border.width: th.style === "editorial" ? 2 : 1
    border.color: th.border

    RowLayout {
        id: row
        anchors.centerIn: parent
        spacing: 2

        Repeater {
            model: bar.styles
            delegate: Rectangle {
                id: seg
                required property int index
                required property string modelData
                readonly property bool on: core.settings.values.viewStyle === modelData
                Layout.preferredWidth: 36
                Layout.preferredHeight: 34
                radius: bar.th.radius === 0 ? 0 : 8
                color: on ? bar.th.accent : (segMouse.containsMouse ? bar.th.cardHover : "transparent")
                Behavior on color { enabled: core.settings.values.animations; ColorAnimation { duration: 120 } }
                Icon {
                    anchors.centerIn: parent
                    name: seg.modelData
                    color: seg.on ? "white" : bar.th.text
                    size: 20
                }
                MouseArea {
                    id: segMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: core.settings.set("viewStyle", seg.modelData)
                }
                ToolTip.visible: segMouse.containsMouse
                ToolTip.delay: 500
                ToolTip.text: modelData.charAt(0).toUpperCase() + modelData.slice(1) + " view"
            }
        }

        Item { Layout.preferredWidth: 6 }
        Slider {
            id: size
            Layout.preferredWidth: 70
            from: 0
            to: 2
            stepSize: 1
            snapMode: Slider.SnapAlways
            value: Math.max(0, bar.sizes.indexOf(core.settings.values.cardSize))
            onMoved: core.settings.set("cardSize", bar.sizes[Math.round(value)])
            ToolTip.visible: hovered
            ToolTip.text: "Card size"
            background: Rectangle {
                x: size.leftPadding
                y: size.topPadding + size.availableHeight / 2 - height / 2
                width: size.availableWidth
                height: 4
                radius: 2
                color: bar.th.border
                Rectangle { width: size.visualPosition * parent.width; height: parent.height; radius: 2; color: bar.th.accent }
            }
            handle: Rectangle {
                x: size.leftPadding + size.visualPosition * (size.availableWidth - width)
                y: size.topPadding + size.availableHeight / 2 - height / 2
                width: 14
                height: 14
                radius: 7
                color: bar.th.accent
            }
        }
    }
}
