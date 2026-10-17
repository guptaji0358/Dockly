import QtQuick
import QtQuick.Layouts
import LH

// Favorites + recent. While searching, shows matches across every hub instead.
Item {
    id: page
    property var host
    property string search: ""
    readonly property bool searching: search.trim() !== ""
    readonly property bool recentOn: core.settings.values.recentEnabled

    ItemModel { id: favModel; mode: LH.ModeFavorites }
    ItemModel { id: recentModel; mode: LH.ModeRecent }
    ItemModel { id: searchModel; search: page.search }

    component Heading: Text {
        color: core.theme.text
        font.pixelSize: 17
        font.weight: Font.DemiBold
    }

    ColumnLayout {
        anchors.fill: parent
        visible: !page.searching
        spacing: 6

        Heading { text: "★  Favorites" }
        ItemGrid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            host: page.host
            model: favModel
            emptyText: "No favorites yet.\nRight-click any card and choose \"Add to Favorites\"."
        }
        Heading { text: "Recent"; visible: page.recentOn; Layout.topMargin: 6 }
        ItemGrid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: page.recentOn
            host: page.host
            model: recentModel
            emptyText: "Items you open will show up here."
        }
    }

    ColumnLayout {
        anchors.fill: parent
        visible: page.searching
        spacing: 6
        Heading { text: "Search results (" + searchModel.count + ")" }
        ItemGrid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            host: page.host
            model: searchModel
            emptyText: "Nothing matches your search."
        }
    }
}
