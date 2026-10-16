/*
 * Author: Runor Ewhro
 * Description: defines the colors shared by the clipboard history interface,
 *              following the system light or dark appearance and taking the
 *              selection color from the system accent.
 */
import QtQuick

QtObject {
    readonly property SystemPalette system: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property bool dark: system.window.hslLightness < 0.5

    readonly property color panel: dark ? "#2a2c30" : "#f8f8f6"
    readonly property color panelEdge: dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.16)
    // text sits on translucent glass over any wallpaper, so both levels keep
    // well above the contrast a solid background would need
    readonly property color foreground: dark ? "#f4f4f6" : "#111214"
    readonly property color muted: dark ? "#b4b8bf" : "#4a4e55"
    readonly property color hairline: dark ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(0.08, 0.09, 0.1, 0.08)
    readonly property color keycap: dark ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(0.08, 0.09, 0.1, 0.06)
    readonly property color hover: dark ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(0, 0, 0, 0.04)
    readonly property color accent: system.highlight
    readonly property color accentForeground: "#ffffff"
    readonly property color accentMuted: Qt.rgba(1, 1, 1, 0.85)
    readonly property color live: dark ? "#3ec46d" : "#2f9e57"
    readonly property color blocked: dark ? "#e0a446" : "#b8741f"

    // code in the preview: colour does two jobs. Structure takes the system
    // accent, so code reads as part of this panel; values take one sand
    // tone; comments fade; names are set in weight, not colour
    // the accent's hue at a lightness that reads on the glass; the system
    // accent itself is a selection colour, too dark for text in dark mode
    readonly property color codeStructure: Qt.hsla(accent.hslHue < 0 ? 0.6 : accent.hslHue,
                                                   Math.min(accent.hslSaturation, 0.8),
                                                   dark ? 0.74 : 0.4, 1)
    readonly property color codeValue: dark ? "#dcbb85" : "#8a5a14"
    readonly property color codeComment: dark ? "#8f949c" : "#6e737b"
    readonly property color codeAdded: live
    readonly property color codeRemoved: dark ? "#e8867a" : "#b3412f"
    // the soft edge where code runs on past the visible width
    readonly property color codeEdge: dark ? Qt.rgba(0, 0, 0, 0.2) : Qt.rgba(0, 0, 0, 0.05)
}
