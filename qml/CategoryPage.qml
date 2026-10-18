import QtQuick
import QtQuick.Layouts
import LH

// One hub: Software, Documents, Entertainment or Web Shortcuts.
Item {
    id: page
    property var host
    property int category: LH.Software

    ItemModel { id: itemModel; category: page.category }

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
