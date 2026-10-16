/*
 * Author: Runor Ewhro
 * Description: shows when the session started, or the size limit as it
 *              nears, beside clickable key hints for the panel's main
 *              actions, so mouse users can do what the keys do.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    property string note: ""
    property bool capturing: true
    property bool hasItems: true
    signal copyRequested()
    signal removeRequested()
    signal pauseRequested()
    implicitHeight: theme.footerHeight

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: colors.hairline
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: theme.sideMargin
        anchors.verticalCenter: parent.verticalCenter
        text: root.note
        color: colors.muted
        font.pixelSize: theme.metaSize
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: theme.sideMargin - 6
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Repeater {
            model: [
                { action: "copy", key: "↩", label: "Copy", needsItems: true },
                { action: "remove", key: "⌘⌫", label: "Remove", needsItems: true },
                { action: "pause", key: "⌘P", label: root.capturing ? "Pause" : "Resume", needsItems: false }
            ]
            delegate: Rectangle {
                id: button
                required property var modelData
                readonly property bool available: !modelData.needsItems || root.hasItems
                width: hint.implicitWidth + 12
                height: 24
                radius: 7
                color: area.containsMouse && available ? colors.hover : "transparent"
                opacity: available ? 1 : 0.4
                scale: area.pressed && available ? 0.94 : 1
                Behavior on color { ColorAnimation { duration: theme.hoverMs } }
                Behavior on opacity { NumberAnimation { duration: theme.hoverMs } }
                Behavior on scale { SpringAnimation { spring: 9; damping: 0.4; epsilon: 0.002 } }

                Row {
                    id: hint
                    anchors.centerIn: parent
                    spacing: 5
                    Rectangle {
                        width: keyText.implicitWidth + 10
                        height: 16
                        radius: 4
                        color: colors.keycap
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            id: keyText
                            anchors.centerIn: parent
                            text: button.modelData.key
                            color: colors.foreground
                            font.pixelSize: theme.keySize
                        }
                    }
                    Text {
                        text: button.modelData.label
                        color: area.containsMouse && button.available ? colors.foreground : colors.muted
                        font.pixelSize: theme.metaSize
                        anchors.verticalCenter: parent.verticalCenter
                        Behavior on color { ColorAnimation { duration: theme.hoverMs } }
                    }
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: button.available ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!button.available) {
                            return;
                        }
                        if (button.modelData.action === "copy") {
                            root.copyRequested();
                        } else if (button.modelData.action === "remove") {
                            root.removeRequested();
                        } else {
                            root.pauseRequested();
                        }
                    }
                }
            }
        }
    }
}
