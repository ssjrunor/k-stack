/*
 * Author: Runor Ewhro
 * Description: defines the colors shared by the clipboard history interface,
 *              following the system light or dark appearance and taking the
 *              selection color from the user's macOS accent.
 */
import QtQuick

QtObject {
    readonly property SystemPalette system: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property bool dark: system.window.hslLightness < 0.5

    readonly property color panel: dark ? "#2a2c30" : "#f8f8f6"
    readonly property color panelEdge: dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.16)
    readonly property color foreground: dark ? "#ececee" : "#1b1d20"
    readonly property color muted: dark ? "#9a9ea6" : "#6c717a"
    readonly property color hairline: dark ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(0.08, 0.09, 0.1, 0.08)
    readonly property color keycap: dark ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(0.08, 0.09, 0.1, 0.06)
    readonly property color hover: dark ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(0, 0, 0, 0.04)
    readonly property color accent: system.highlight
    readonly property color accentForeground: "#ffffff"
    readonly property color accentMuted: Qt.rgba(1, 1, 1, 0.78)
    readonly property color live: dark ? "#3ec46d" : "#2f9e57"
    readonly property color blocked: dark ? "#e0a446" : "#b8741f"
}
