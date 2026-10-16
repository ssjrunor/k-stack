/*
 * Author: Runor Ewhro
 * Description: renders one captured item as a panel row with a glyph or
 *              color swatch for its kind, a title that hides possible
 *              passwords, an optional subtext such as a translation, an
 *              optional detail line, hover and press feedback,
 *              a copied confirmation, and a collapse when it is removed.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    // filled from the model's roles when this row is a list delegate; the
    // preview pane reads them from the current row as well
    required property string contentText
    required property var entryId
    required property string capturedAt
    required property string kind
    required property string kindLabel
    required property string title
    required property string titleTail
    required property string detail
    required property string actionLabel
    required property string actionUrl
    required property string actionText
    required property string actionKind
    required property color swatch
    required property bool sensitive
    required property bool monospace
    required property string when
    required property bool allDay
    required property string whenLabel
    // an optional second line any kind can fill in, such as a translation
    required property string subtext

    property bool selected: false
    property bool copied: false
    property bool detailed: false
    property bool reduceMotion: false
    readonly property bool pressed: mouse.pressed
    signal copyRequested()
    signal removeRequested()
    signal hovered()
    height: (detailed ? theme.detailedRowHeight : theme.rowHeight)
            + (subtext.length > 0 ? theme.subtextHeight : 0)
    // a subtext can arrive after the row appears; the row opens to fit it
    Behavior on height {
        enabled: !root.reduceMotion
        NumberAnimation { duration: theme.hoverMs; easing.type: Easing.OutCubic }
    }
    clip: true

    Style.Colors { id: colors }
    Style.Theme { id: theme }

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

        KindGlyph {
            id: glyph
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            kind: root.kind
            swatch: root.swatch
            tint: root.metaColor
        }

        Column {
            anchors.left: glyph.right
            anchors.leftMargin: 10
            anchors.right: meta.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Text {
                width: parent.width
                textFormat: root.titleTail.length > 0 ? Text.StyledText : Text.PlainText
                text: root.titleTail.length > 0
                      ? root.escaped(root.title) + " <font color=\"" + root.metaColor + "\">"
                        + root.escaped(root.titleTail) + "</font>"
                      : root.title
                color: root.inkColor
                font.pixelSize: root.monospace ? theme.codeSize : theme.bodySize
                font.family: root.monospace ? theme.monoFamily : Qt.application.font.family
                font.letterSpacing: root.kind === "verification" || root.sensitive ? 1.5 : 0
                elide: Text.ElideRight
                maximumLineCount: 1
                Behavior on color { ColorAnimation { duration: theme.hoverMs } }
            }

            Text {
                width: parent.width
                visible: root.subtext.length > 0
                text: root.subtext
                color: root.metaColor
                font.pixelSize: theme.metaSize
                elide: Text.ElideRight
                maximumLineCount: 1
                Behavior on color { ColorAnimation { duration: theme.hoverMs } }
            }

            Text {
                width: parent.width
                visible: root.detailed
                // the subtext may already name the kind, such as a code's language
                text: root.kindLabel === root.detail ? root.kindLabel
                      : root.subtext === root.kindLabel ? root.detail
                      : root.kindLabel + ", " + root.detail
                color: root.metaColor
                font.pixelSize: theme.metaSize
                elide: Text.ElideRight
                maximumLineCount: 1
                Behavior on color { ColorAnimation { duration: theme.hoverMs } }
            }
        }

        Text {
            id: meta
            // remove sits outside content, so it cannot be anchored to;
            // content fills the row, so its edge is the same line
            anchors.right: parent.right
            anchors.rightMargin: remove.shown ? remove.width + 16 : 10
            Behavior on anchors.rightMargin {
                enabled: !root.reduceMotion
                NumberAnimation { duration: theme.hoverMs; easing.type: Easing.OutCubic }
            }
            anchors.verticalCenter: parent.verticalCenter
            text: root.copied ? "✓ Copied" : root.selected ? platformUi.keyText("Return") : root.timeText
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
    // remove the selected row with the remove shortcut
    Rectangle {
        id: remove
        // stays visible while fading out, so it slides away instead of vanishing
        readonly property bool shown: mouse.containsMouse || removeArea.containsMouse
        visible: opacity > 0
        opacity: shown ? 1 : 0
        width: 20
        height: 20
        radius: 10
        anchors.right: parent.right
        anchors.rightMargin: shown ? 8 : 0
        Behavior on opacity { NumberAnimation { duration: root.reduceMotion ? 0 : theme.hoverMs } }
        Behavior on anchors.rightMargin {
            enabled: !root.reduceMotion
            NumberAnimation { duration: theme.hoverMs; easing.type: Easing.OutCubic }
        }
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
