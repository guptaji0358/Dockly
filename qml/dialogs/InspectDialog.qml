import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Stage 2 "Inspect Processes": read-only view of the processes that belong to a program card, with the
// evidence behind each decision. Nothing here can end, block or change a process.
AppDialog {
    id: dlg
    property string itemId
    property string itemName
    property var result: null
    property bool showOthers: false
    property var shown: []
    property var sel: null
    width: Math.min(900, (Overlay.overlay ? Overlay.overlay.width : 900) - 40)

    function openFor(id, name) {
        itemId = id; itemName = name; result = null; sel = null; shown = []
        title = "Inspect processes: " + name
        open()
        core.inspectProcesses(id)
    }
    function rebuild() {
        const out = []
        if (result)
            for (const r of result.rows)
                if (showOthers || r.related)
                    out.push(r)
        const rank = {main: 0, essential: 1, candidate: 2, unknown: 3}
        out.sort((a, b) => (b.mine - a.mine) || (rank[a.classKey] - rank[b.classKey]) || (a.pid - b.pid))
        shown = out
        sel = out.length ? out[0] : null
    }
    onShowOthersChanged: rebuild()
    function colorFor(k) {
        return k === "main" ? "#3fb950" : k === "essential" ? "#58a6ff" : k === "candidate" ? "#d29922" : dlg.th.subtext
    }
    Connections {
        target: core
        function onProcessInspection(id, r) {
            if (id !== dlg.itemId) return
            dlg.result = r
            dlg.rebuild()
        }
    }

    Text {
        Layout.fillWidth: true
        color: dlg.th.subtext
        wrapMode: Text.Wrap
        text: !dlg.result ? "Reading the process list..."
              : dlg.result.error !== "" ? dlg.result.error
              : "Read-only. " + dlg.result.main + " main, " + dlg.result.essential + " essential, "
                + dlg.result.candidate + " candidate-background. " + dlg.result.processCount
                + " processes looked at in " + dlg.result.elapsedMs + " ms. Nothing is ended, blocked or changed."
    }
    CheckBox {
        text: "Also list unrelated and unknown processes"
        checked: dlg.showOthers
        onToggled: dlg.showOthers = checked
        contentItem: Text { text: parent.text; leftPadding: parent.indicator.width + 8; color: dlg.th.text; verticalAlignment: Text.AlignVCenter }
    }
    Text {
        visible: dlg.result && dlg.shown.length === 0
        text: "No running process was attributed to this program."
        color: dlg.th.text
    }
    ListView {
        id: list
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(240, Math.max(40, contentHeight))
        clip: true
        model: dlg.shown
        ScrollBar.vertical: ScrollBar {}
        delegate: Rectangle {
            required property var modelData
            width: list.width
            height: 30
            color: dlg.sel && dlg.sel.key === modelData.key ? Qt.rgba(dlg.th.accent.r, dlg.th.accent.g, dlg.th.accent.b, 0.22) : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8; anchors.rightMargin: 8
                spacing: 10
                Text { text: modelData.name; color: dlg.th.text; elide: Text.ElideRight; Layout.preferredWidth: 220 }
                Text { text: "PID " + modelData.pid + " / parent " + modelData.ppid; color: dlg.th.subtext; Layout.preferredWidth: 170 }
                Text { text: modelData.classLabel; color: dlg.colorFor(modelData.classKey); font.bold: true; Layout.preferredWidth: 140 }
                Text { text: modelData.cardName !== "" ? modelData.cardName : (modelData.ambiguous.length ? "ambiguous" : "-"); color: dlg.th.subtext; elide: Text.ElideRight; Layout.fillWidth: true }
            }
            MouseArea { anchors.fill: parent; onClicked: dlg.sel = modelData }
        }
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: Math.max(detail.implicitHeight + 20, 190)
        radius: 8
        color: Qt.rgba(dlg.th.text.r, dlg.th.text.g, dlg.th.text.b, 0.05)
        visible: dlg.sel !== null
        ColumnLayout {
            id: detail
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
            spacing: 4
            Text { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; color: dlg.th.text; font.bold: true
                   text: dlg.sel ? dlg.sel.name + "  -  " + dlg.sel.classLabel : "" }
            Text { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; color: dlg.th.subtext
                   text: dlg.sel ? "Path: " + (dlg.sel.exePath || "(not readable)") : "" }
            Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: dlg.th.subtext
                   text: dlg.sel ? "PID " + dlg.sel.pid + ", parent PID " + dlg.sel.ppid
                         + (dlg.sel.parentName ? " (" + dlg.sel.parentName + ")" : "")
                         + (dlg.sel.parentValid ? "" : " - parent not valid (exited or PID reused)")
                         + (dlg.sel.created ? ", started " + dlg.sel.created : "") : "" }
            Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: dlg.th.subtext
                   text: dlg.sel ? "Software card: " + (dlg.sel.cardName || "none") : "" }
            Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: dlg.th.text
                   text: dlg.sel && dlg.sel.evidence.length ? "Evidence:\n - " + dlg.sel.evidence.join("\n - ") : "Evidence: none" }
            Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: dlg.th.text
                   text: dlg.sel ? "Why: " + dlg.sel.reason : "" }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        Item { Layout.fillWidth: true }
        AppButton { text: "Refresh"; onClicked: { dlg.result = null; core.inspectProcesses(dlg.itemId) } }
        AppButton { text: "Close"; primary: true; onClicked: dlg.close() }
    }
}
