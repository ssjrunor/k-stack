/*
 * Author: Runor Ewhro
 * Description: shows the categories present in the current results as chips
 *              with counts, so the list can be narrowed to links, colors,
 *              contacts, and so on; Tab and Shift-Tab move between them.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    // each entry is {id, label, count} from the results model
    property var categories: []
    property int total: 0
    property string current: ""
    property bool reduceMotion: false
    signal chosen(string id)
    implicitHeight: theme.filterHeight

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    readonly property var chips: [{ id: "", label: "All", count: total }].concat(categories)

    // moves to the next or previous chip, wrapping at either end
    function step(delta) {
        let at = 0;
        for (let i = 0; i < chips.length; ++i) {
            if (chips[i].id === current) {
                at = i;
            }
        }
        chosen(chips[(at + delta + chips.length) % chips.length].id);
    }

    Flickable {
        id: strip
        anchors.fill: parent
        anchors.leftMargin: theme.sideMargin - 8
        anchors.rightMargin: theme.sideMargin - 8
        contentWidth: row.implicitWidth
        contentHeight: height
        clip: true
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentWidth > width

        Row {
            id: row
            height: parent.height
            spacing: 4

            Repeater {
                model: root.chips
                delegate: Rectangle {
                    id: chip
                    required property var modelData
                    readonly property bool active: modelData.id === root.current
                    anchors.verticalCenter: parent.verticalCenter
                    width: label.implicitWidth + 22
                    height: 26
                    radius: height / 2
                    color: active ? colors.keycap : area.containsMouse ? colors.hover : "transparent"
                    border.width: active ? 1 : 0
                    border.color: colors.hairline
                    scale: area.pressed ? 0.95 : 1
                    Behavior on color { ColorAnimation { duration: theme.hoverMs } }
                    Behavior on scale {
                        enabled: !root.reduceMotion
                        SpringAnimation { spring: 9; damping: 0.4; epsilon: 0.002 }
                    }

                    // keep the chosen chip in view as Tab moves along the strip
                    onActiveChanged: {
                        if (active && strip.interactive) {
                            strip.contentX = Math.max(0, Math.min(strip.contentWidth - strip.width,
                                                                  x - strip.width / 2 + width / 2));
                        }
                    }

                    Text {
                        id: label
                        anchors.centerIn: parent
                        textFormat: Text.StyledText
                        text: chip.modelData.label + "&nbsp;&nbsp;<font color=\"" + colors.muted + "\">"
                              + chip.modelData.count + "</font>"
                        color: chip.active ? colors.foreground : colors.muted
                        font.pixelSize: theme.metaSize + 1
                        font.weight: chip.active ? Font.DemiBold : Font.Normal
                        font.features: { "tnum": 1 }
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.chosen(chip.modelData.id)
                    }
                }
            }
        }
    }
}
