/*
 * Author: Runor Ewhro
 * Description: reserves the compact search window for the floating interface.
 */
import QtQuick
import QtQuick.Window
import "../components" as Components

Window {
    width: 460
    height: 90
    visible: false
    title: "Search"
    Components.SearchField { anchors.fill: parent; anchors.margins: 16 }
}
