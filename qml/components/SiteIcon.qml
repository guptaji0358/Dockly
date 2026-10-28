import QtQuick

// A site's icon: its own picture when it has one, otherwise the tinted icon-pack glyph.
Item {
    id: root
    property string image: ""   // file URL of the user's picture
    property string name: "web" // icon-pack glyph used when there is no picture
    property color color: "white"
    property int size: 20

    width: size
    height: size

    Image {
        anchors.fill: parent
        visible: root.image !== ""
        source: root.image
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
        mipmap: true
    }
    Icon {
        visible: root.image === ""
        anchors.fill: parent
        name: root.name
        size: root.size
        color: root.color
    }
}
