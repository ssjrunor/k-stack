/*
 * Author: Runor Ewhro
 * Description: renders one captured item as a single panel row, showing links
 *              as host and path and code-like text in monospace, with its
 *              observation time, hover and press feedback, a copied
 *              confirmation, and a collapse when it is removed.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    // filled from the model's roles when this row is a list delegate
    required property string contentText
    required property var entryId
    required property string capturedAt
    property bool selected: false
    property bool copied: false
    property bool reduceMotion: false
    readonly property bool pressed: mouse.pressed
    signal copyRequested()
    signal removeRequested()
    signal hovered()
    height: theme.rowHeight
    clip: true

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    readonly property string flat: contentText.trim().replace(/\s+/g, " ")
    readonly property var link: flat.match(/^https?:\/\/([^\/\s?#]+)(\S*)$/)
    readonly property bool isCode: !link && flat.length < 200
                                   && (/^#[0-9A-Fa-f]{3,8}$/.test(flat) || /(^|\s)--?\w|[{};]|=>|::|^\$ /.test(flat))
    readonly property string timeText: capturedAt.length > 0
                                       ? new Date(capturedAt).toLocaleTimeString(Qt.locale(), Locale.ShortFormat) : ""
    readonly property color inkColor: selected ? colors.accentForeground : colors.foreground
    readonly property color metaColor: selected ? colors.accentMuted : colors.muted

    function escaped(value) {
        return value.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    // fold the row away first so the rows below slide up into its place,
    // then ask for the removal
    function collapseAndRemove() {
        if (!collapse.running) {
            collapse.start();
        }
    }

    SequentialAnimation {
        id: collapse
        ParallelAnimation {
            NumberAnimation { target: root; property: "opacity"; to: 0; duration: theme.collapseMs * 0.6 }
            NumberAnimation {
                target: root
                property: "height"
                to: 0
                duration: root.reduceMotion ? 0 : theme.collapseMs
                easing.type: Easing.InOutCubic
            }
        }
        ScriptAction { script: root.removeRequested() }
    }

    // the selected row's fill is the list's shared highlight; this layer only
    // shows hover on the other rows
    Rectangle {
        anchors.fill: parent
        radius: theme.rowRadius
        color: !root.selected && mouse.containsMouse ? colors.hover : "transparent"
        Behavior on color { ColorAnimation { duration: theme.hoverMs } }
    }

    Item {
        id: content
        anchors.fill: parent
        scale: root.pressed ? 0.985 : 1
        Behavior on scale {
            enabled: !root.reduceMotion
            SpringAnimation { spring: 8; damping: 0.45; epsilon: 0.002 }
        }

        Text {
            id: label
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: meta.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            textFormat: root.link ? Text.StyledText : Text.PlainText
            text: root.link
                  ? root.escaped(root.link[1].replace(/^www\./, "")) + " <font color=\""
                    + root.metaColor + "\">" + root.escaped(root.link[2]) + "</font>"
                  : root.flat
            color: root.inkColor
            font.pixelSize: root.isCode ? theme.codeSize : theme.bodySize
            font.family: root.isCode ? theme.monoFamily : Qt.application.font.family
            elide: Text.ElideRight
            maximumLineCount: 1
            Behavior on color { ColorAnimation { duration: theme.hoverMs } }
        }

        Text {
            id: meta
            anchors.right: remove.visible ? remove.left : parent.right
            anchors.rightMargin: remove.visible ? 8 : 10
            anchors.verticalCenter: parent.verticalCenter
            text: root.copied ? "✓ Copied" : root.selected ? "↩" : root.timeText
            color: root.metaColor
            font.pixelSize: theme.metaSize
            font.weight: root.copied ? Font.DemiBold : Font.Normal
            font.features: { "tnum": 1 }
            Behavior on color { ColorAnimation { duration: theme.hoverMs } }
        }

        // a small pop when the copy lands, so the confirmation is noticed
        // in the moment before the panel closes
        SequentialAnimation {
            running: root.copied && !root.reduceMotion
            NumberAnimation { target: meta; property: "scale"; from: 0.7; to: 1; duration: 260; easing.type: Easing.OutBack; easing.overshoot: 2.2 }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        // follow real pointer movement only, so rows sliding under a resting
        // pointer while typing do not steal the selection
        onPositionChanged: root.hovered()
        onClicked: root.copyRequested()
    }

    // mouse users get a remove control on the hovered row; keyboard users
    // remove the selected row with Command-Backspace
    Rectangle {
        id: remove
        visible: mouse.containsMouse || removeArea.containsMouse
        width: 20
        height: 20
        radius: 10
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        color: removeArea.containsMouse ? (root.selected ? Qt.rgba(1, 1, 1, 0.2) : colors.keycap) : "transparent"
        scale: removeArea.pressed ? 0.88 : 1
        Behavior on color { ColorAnimation { duration: theme.hoverMs } }
        Behavior on scale { enabled: !root.reduceMotion; SpringAnimation { spring: 9; damping: 0.4; epsilon: 0.002 } }

        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -1
            text: "×"
            color: root.metaColor
            font.pixelSize: 15
        }
        MouseArea {
            id: removeArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.collapseAndRemove()
        }
    }
}
