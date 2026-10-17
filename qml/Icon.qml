import QtQuick

// A custom SVG icon from the app's icon pack, tinted with `color`.
Image {
    id: icon
    property string name
    property color color: "white"
    property int size: 20

    width: size
    height: size
    sourceSize: Qt.size(size * 2, size * 2)
    fillMode: Image.PreserveAspectFit
    smooth: true
    // QColor's hex string is #rrggbb or #aarrggbb; the provider wants it without the '#'.
    source: name === "" ? "" : "image://icon/" + name + "?c=" + color.toString().substring(1)
}
