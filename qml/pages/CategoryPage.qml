import QtQuick
import QtQuick.Layouts
import LH

// One hub: Software, Documents, Entertainment or Web Shortcuts.
Item {
    id: page
    property var host
    property int category: LH.Software

    property string tag: "" // active tag filter ("" = all)
    property var tags: core.tagList(page.category)
    ItemModel { id: itemModel; category: page.category; tag: page.tag }
    Connections {
        target: core
        function onItemsChanged() {
            page.tags = core.tagList(page.category)
            if (page.tag !== "" && !page.tags.some(t => t.tag === page.tag))
                page.tag = "" // the last item with this tag lost it
        }
    }

    readonly property string title: category === LH.Software ? "Software Hub"
                                  : category === LH.Document ? "Documents Hub"
                                  : category === LH.Entertainment ? "Entertainment Hub" : "Web Shortcuts"
    readonly property string empty: category === LH.Software
        ? "No programs added yet.\nClick \"Add Software\" to pick from the detected programs or browse for an .exe."
        : category === LH.Document ? "No documents yet.\nClick \"Add Document\" to pin files here."
        : category === LH.Entertainment ? "No media yet.\nClick \"Add Media\" to add videos, music or images."
        : "No shortcuts yet.\nClick \"Add URL\" to pin a website."

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: page.title
                color: core.theme.text
                font.pixelSize: 26
                font.weight: Font.DemiBold
                font.family: core.theme.headingFont
            }
            Text {
                text: itemModel.count + " item(s)"
                color: core.theme.subtext
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: 4
            }
            Item { Layout.fillWidth: true }

            AppButton {
                visible: page.category === LH.Software
                iconName: "scan"
                text: core.scanning ? "Scanning..." : "Scan this PC"
                enabled: !core.scanning
                onClicked: core.refreshSoftware()
            }
            AppButton {
                primary: true
                iconName: "add"
                text: page.category === LH.Software ? "Add Software"
                    : page.category === LH.Document ? "Add Document"
                    : page.category === LH.Entertainment ? "Add Media" : "Add URL"
                onClicked: {
                    if (page.category === LH.Web)
                        page.host.addUrl()
                    else if (page.category === LH.Software)
                        page.host.addSoftware()
                    else
                        page.host.addFiles(page.category)
                }
            }
        }

        // tag filter: click a tag to narrow the hub to it, click again (or "All") to clear
        Flow {
            Layout.fillWidth: true
            spacing: 6
            visible: page.tags.length > 0
            Repeater {
                model: [{tag: "", count: 0}].concat(page.tags)
                delegate: Rectangle {
                    id: chip
                    required property var modelData
                    readonly property bool on: page.tag === modelData.tag
                    height: 26
                    width: chipText.implicitWidth + 20
                    radius: core.theme.radius === 0 ? 0 : 13
                    color: on ? core.theme.accent : (chipMouse.containsMouse ? core.theme.cardHover : "transparent")
                    border.width: 1
                    border.color: on ? core.theme.accent : core.theme.border
                    Text {
                        id: chipText
                        anchors.centerIn: parent
                        text: chip.modelData.tag === "" ? "All" : "#" + chip.modelData.tag
                        color: chip.on ? "white" : core.theme.text
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                    MouseArea {
                        id: chipMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: page.tag = chip.on ? "" : chip.modelData.tag
                    }
                }
            }
        }

        ItemSpace {
            Layout.fillWidth: true
            Layout.fillHeight: true
            host: page.host
            model: itemModel
            coreLabel: page.title.toUpperCase()
            emptyText: page.empty
        }
    }
}
