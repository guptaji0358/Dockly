pragma Singleton
import QtQuick

// One place that decides which GPU effects are on, from Settings > Performance.
//   low       flat gradient background, no motion effects  (lightest)
//   balanced  animated aurora shader (30 fps) + tilt and floating tiles
//   high      aurora at full rate, star field, parallax, glow halos
QtObject {
    id: fx

    // set by Main.qml
    property bool windowActive: true
    property bool softwareRenderer: false

    readonly property var s: core.settings.values
    readonly property string level: s.gpuLevel
    readonly property bool animated: s.animations
    // effects run only while the window is in front (when the user wants that)
    readonly property bool live: animated && (windowActive || !s.pauseWhenInactive)

    readonly property bool aurora: !softwareRenderer && level !== "low"
    readonly property bool auroraLive: aurora && live
    readonly property int auroraInterval: level === "high" ? 16 : 33
    readonly property bool stars: level === "high" && !softwareRenderer
    readonly property bool parallax: level === "high" && animated
    readonly property bool glow: level !== "low"
    readonly property bool haloGlow: level === "high"
    readonly property bool tilt: level !== "low" && animated
    readonly property bool floating: level !== "low" && animated && s.floatingTiles
    readonly property bool spin: level === "high" && animated   // orbit rings slowly turn
}
