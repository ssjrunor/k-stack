/*
 * Author: Runor Ewhro
 * Description: shows when the session started, or the size limit as it
 *              nears, beside clickable key hints for the panel's actions:
 *              copy, the selected entry's suggested action, the preview,
 *              and pausing capture.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    property string note: ""
    property bool capturing: true
    property bool hasItems: true
    property bool previewVisible: false
    property string actionLabel: ""
    property bool reduceMotion: false
    signal copyRequested()
    signal actionRequested()
    signal previewRequested()
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
        anchors.right: hints.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: root.note
        color: colors.muted
        font.pixelSize: theme.metaSize
        elide: Text.ElideRight
    }

    Row {
        id: hints
        anchors.right: parent.right
        anchors.rightMargin: theme.sideMargin - 6
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Repeater {
            model: [
                { action: "copy", key: platformUi.keyText("Return"), label: "Copy", needsItems: true, shown: true },
                { action: "suggest", key: platformUi.keyText("Ctrl+O"), label: root.actionLabel, needsItems: true,
                  shown: root.actionLabel.length > 0 },
                { action: "preview", key: platformUi.keyText("Ctrl+Y"), label: root.previewVisible ? "Hide preview" : "Preview",
                  needsItems: false, shown: true },
                { action: "pause", key: platformUi.keyText("Ctrl+P"), label: root.capturing ? "Pause" : "Resume",
                  needsItems: false, shown: true }
            ]
            delegate: Rectangle {
                id: button
                required property var modelData
                readonly property bool available: !modelData.needsItems || root.hasItems
                visible: modelData.shown
                width: hint.implicitWidth + 12
                height: 24
                radius: 7
                color: area.containsMouse && available ? colors.hover : "transparent"
                opacity: available ? 1 : 0.4
                scale: area.pressed && available ? 0.94 : 1
                Behavior on color { ColorAnimation { duration: theme.hoverMs } }
                Behavior on opacity { NumberAnimation { duration: theme.hoverMs } }
                Behavior on scale {
                    enabled: !root.reduceMotion
                    SpringAnimation { spring: 9; damping: 0.4; epsilon: 0.002 }
                }

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
                        const action = button.modelData.action;
                        if (action === "copy") {
                            root.copyRequested();
                        } else if (action === "suggest") {
                            root.actionRequested();
                        } else if (action === "preview") {
                            root.previewRequested();
                        } else {
                            root.pauseRequested();
                        }
                    }
                }
            }
        }
    }
}
