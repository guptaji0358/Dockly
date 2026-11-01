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
    // "#software photo" = only Software cards matching "photo"; a bare "#" lists everything.
    // Unknown tags match nothing (-2), so a typo never silently searches all hubs.
    readonly property var parsed: {
        const raw = page.search.trim()
        if (raw.charAt(0) !== "#")
            return {cat: LH.AllCategories, rest: raw, tag: ""}
        const m = /^#(\S*)\s*(.*)$/.exec(raw)
        const word = m[1].toLowerCase()
        if (word === "")
            return {cat: LH.AllCategories, rest: m[2], tag: ""}
        const tags = page.host.allSearchTags
        // an exact name wins over a prefix ("#web" is the category even though a tag "website" exists)
        for (let i = 0; i < tags.length; ++i)
            if (tags[i].key === word)
                return tags[i].tag ? {cat: LH.AllCategories, rest: m[2], tag: tags[i].key} : {cat: tags[i].cat, rest: m[2], tag: ""}
        for (let i = 0; i < tags.length; ++i)
            if (tags[i].key.indexOf(word) === 0 || (tags[i].alias !== "" && tags[i].alias.indexOf(word) === 0))
                return tags[i].tag ? {cat: LH.AllCategories, rest: m[2], tag: tags[i].key} : {cat: tags[i].cat, rest: m[2], tag: ""}
        return {cat: -2, rest: m[2], tag: ""}
    }

    ItemModel { id: searchModel; category: page.parsed.cat; search: page.parsed.rest; tag: page.parsed.tag }

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
