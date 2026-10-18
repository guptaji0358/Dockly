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
        color: core.theme.accent
        font.pixelSize: 12
        font.weight: Font.DemiBold
        font.letterSpacing: 4
    }

    ColumnLayout {
        anchors.fill: parent
        visible: !page.searching
        spacing: 6

        Heading { text: "★  FAVORITES" }
        ItemSpace {
            Layout.fillWidth: true
            Layout.fillHeight: true
            host: page.host
            model: favModel
            coreLabel: "FAVORITES"
            emptyText: "No favorites yet.\nRight-click any card and choose \"Add to Favorites\"."
        }
        Heading { text: "RECENT"; visible: page.recentOn; Layout.topMargin: 6 }
        ItemSpace {
            Layout.fillWidth: true
            Layout.preferredHeight: 128
            strip: true
            visible: page.recentOn
            host: page.host
            model: recentModel
            coreLabel: "RECENT"
            emptyText: "Items you open will show up here."
        }
    }

    ColumnLayout {
        anchors.fill: parent
        visible: page.searching
        spacing: 6
        Heading { text: "Search results (" + searchModel.count + ")" }
        ItemSpace {
            Layout.fillWidth: true
            Layout.fillHeight: true
            host: page.host
            model: searchModel
            coreLabel: "RESULTS"
            emptyText: "Nothing matches your search."
        }
    }
}
