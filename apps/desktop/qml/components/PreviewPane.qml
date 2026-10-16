/*
 * Author: Runor Ewhro
 * Description: shows the selected entry in full beside the list, laid out
 *              for its kind: a swatch with conversions for colors, a
 *              calendar leaf for dates, the resolved day for times, code as
 *              written and coloured, and possible passwords hidden until shown. Text in
 *              another language can be swapped for its translation. Below
 *              it, one aligned list holds every action the platform's system
 *              services support, opening a title field only when one needs it.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    // the list's current row, or null when nothing is selected
    property var item: null
    property color accent: colors.accent
    property bool reduceMotion: false
    // capability flags from the platform's system services
    property bool canShare: false
    property bool canAddEvent: false
    property bool canShowCalendarDay: false
    property bool canAddReminder: false
    // the outcome of the last share or scheduling request
    property string status: ""
    property bool statusOk: true
    signal copyRequested()
    signal actionRequested()
    signal copyTextRequested(string text)
    signal shareRequested(real x, real y)
    signal eventRequested(string title)
    signal reminderRequested(string title)
    signal calendarDayRequested()
    // gives keys back to the search field after typing a title
    signal doneEditing()

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    property bool revealed: false
    // the action whose title field is open, if any
    property string composing: ""
    // the selected text's translation, shown in place of the text on request
    property string translation: ""
    property bool translating: false
    property bool showingTranslation: false
    onItemChanged: {
        revealed = false;
        status = "";
        composing = "";
        translation = "";
        translating = false;
        showingTranslation = false;
    }

    readonly property string kind: item ? item.kind : ""
    readonly property string text: item ? item.contentText : ""
    readonly property var dateParts: kind === "date" && item ? item.detail.split(", ") : []
    readonly property bool hasWhen: item !== null && item.when.length > 0
    // the language the text can be translated from, such as "German"; empty
    // when there is nothing to translate or no model for it
    readonly property string translatableFrom: kind === "text" && item
                                               ? translationController.translatableFrom(text) : ""

    // every action for the selected entry, in one order: copy first, then the
    // kind's suggestion, sharing, and scheduling
    readonly property var actions: {
        if (!item) {
            return [];
        }
        const list = [{ id: "copy", label: "Copy", glyph: "action-copy", key: platformUi.keyText("Return") }];
        if (item.actionLabel.length > 0) {
            const glyphs = { open: "action-open", copy: "action-copy", map: "address", remove: "action-remove" };
            list.push({ id: "suggest", label: item.actionLabel, glyph: glyphs[item.actionKind] || "action-open",
                        key: platformUi.keyText("Ctrl+O") });
        }
        if (canShare && !item.sensitive) {
            list.push({ id: "share", label: "Share…", glyph: "action-share", key: platformUi.keyText("Ctrl+Shift+S") });
        }
        if (hasWhen && canAddEvent) {
            list.push({ id: "event", label: "Add to calendar…", glyph: "date", key: "" });
        }
        if (hasWhen && canAddReminder) {
            list.push({ id: "reminder", label: "Remind me…", glyph: "action-reminder", key: "" });
        }
        if (hasWhen && canShowCalendarDay) {
            list.push({ id: "day", label: "Show in calendar", glyph: "action-day", key: "" });
        }
        return list;
    }

    function hex2(value) {
        const hex = Math.round(value * 255).toString(16).toUpperCase();
        return hex.length === 1 ? "0" + hex : hex;
    }
    function colorForms(c) {
        const forms = ["#" + hex2(c.r) + hex2(c.g) + hex2(c.b),
                       "rgb(" + Math.round(c.r * 255) + ", " + Math.round(c.g * 255) + ", " + Math.round(c.b * 255) + ")"];
        const hue = c.hslHue < 0 ? 0 : Math.round(c.hslHue * 360);
        forms.push("hsl(" + hue + ", " + Math.round(c.hslSaturation * 100) + "%, " + Math.round(c.hslLightness * 100) + "%)");
        return forms;
    }
    function facts(value) {
        const trimmed = value.trim();
        const words = trimmed.length === 0 ? 0 : trimmed.split(/\s+/).length;
        const lines = trimmed.length === 0 ? 0 : trimmed.split("\n").length;
        return [value.length + (value.length === 1 ? " character" : " characters"),
                words + (words === 1 ? " word" : " words"),
                lines + (lines === 1 ? " line" : " lines")].join("   ");
    }

    function trigger(action, anchor) {
        if (action.id === "copy") {
            copyRequested();
        } else if (action.id === "suggest") {
            actionRequested();
        } else if (action.id === "share") {
            const point = anchor.mapToItem(null, anchor.width / 2, anchor.height);
            shareRequested(point.x, point.y);
        } else if (action.id === "event" || action.id === "reminder") {
            composing = composing === action.id ? "" : action.id;
        } else if (action.id === "day") {
            calendarDayRequested();
        }
    }

    // the first press translates; later presses switch between the
    // translation and the original without translating again
    function toggleTranslation() {
        if (showingTranslation) {
            showingTranslation = false;
        } else if (translation.length > 0) {
            showingTranslation = true;
        } else if (!translating) {
            translating = true;
            status = "";
            translationController.translate(item.entryId);
        }
    }

    Connections {
        target: translationController
        function onTranslated(id, text) {
            if (root.item && root.translating && id == root.item.entryId) {
                root.translation = text;
                root.translating = false;
                root.showingTranslation = true;
            }
        }
        function onTranslationFailed(id) {
            if (root.item && root.translating && id == root.item.entryId) {
                root.translating = false;
                root.statusOk = false;
                root.status = "This text could not be translated.";
            }
        }
    }

    function confirm(actionId, title) {
        composing = "";
        if (actionId === "event") {
            eventRequested(title);
        } else {
            reminderRequested(title);
        }
        doneEditing();
    }

    // a copyable line, used for a color's hex, RGB, and HSL forms
    component CopyLine: Item {
        id: copyLine
        property string value: ""
        width: parent.width
        height: 30
        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: copyLine.value
            color: colors.foreground
            font.family: theme.monoFamily
            font.pixelSize: theme.codeSize
        }
        Text {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: "Copy"
            color: lineArea.containsMouse ? colors.foreground : colors.muted
            font.pixelSize: theme.metaSize
            MouseArea {
                id: lineArea
                anchors.fill: parent
                anchors.margins: -6
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.copyTextRequested(copyLine.value)
            }
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: colors.hairline }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        width: 1
        color: colors.hairline
    }

    Text {
        anchors.centerIn: parent
        visible: !root.item
        text: "Select an item to preview it"
        color: colors.muted
        font.pixelSize: theme.metaSize + 1
    }

    // ---- content: scrolls above the pinned actions ----
    Flickable {
        id: scroller
        visible: root.item !== null
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: actionArea.top
        anchors.topMargin: 18
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.bottomMargin: 10
        contentHeight: body.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: body
            width: scroller.width
            spacing: 14

            // kind and capture time
            Item {
                id: headerRow
                width: parent.width
                height: 20
                KindGlyph {
                    id: headerGlyph
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    kind: root.kind
                    swatch: root.item ? root.item.swatch : "transparent"
                }
                Text {
                    anchors.left: headerGlyph.right
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.item ? root.item.kindLabel : ""
                    color: colors.foreground
                    font.pixelSize: theme.metaSize + 1
                    font.weight: Font.DemiBold
                }
                // translate: swaps the text below for its translation and back
                Rectangle {
                    id: translateButton
                    visible: root.translatableFrom.length > 0
                    anchors.right: copiedLabel.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24
                    height: 24
                    radius: 7
                    color: root.showingTranslation ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.18)
                         : translateArea.containsMouse ? colors.hover : "transparent"
                    scale: translateArea.pressed ? 0.9 : 1
                    Accessible.role: Accessible.Button
                    Accessible.name: root.showingTranslation ? "Show original" : "Translate from " + root.translatableFrom
                    Accessible.onPressAction: root.toggleTranslation()
                    Behavior on color { ColorAnimation { duration: theme.hoverMs } }
                    Behavior on scale {
                        enabled: !root.reduceMotion
                        SpringAnimation { spring: 9; damping: 0.4; epsilon: 0.002 }
                    }

                    KindGlyph {
                        id: translateGlyph
                        anchors.centerIn: parent
                        kind: "action-translate"
                        tint: root.showingTranslation ? root.accent
                              : translateArea.containsMouse ? colors.foreground : colors.muted
                        // breathes while the translation is on its way
                        SequentialAnimation on opacity {
                            running: root.translating && !root.reduceMotion
                            loops: Animation.Infinite
                            onRunningChanged: if (!running) translateGlyph.opacity = 1
                            NumberAnimation { to: 0.35; duration: 420; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 1; duration: 420; easing.type: Easing.InOutSine }
                        }
                    }
                    MouseArea {
                        id: translateArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.toggleTranslation()
                    }
                }
                Text {
                    id: copiedLabel
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.item ? "Copied " + root.item.timeText : ""
                    color: colors.muted
                    font.pixelSize: theme.metaSize
                }
            }

            // color: the swatch itself and its forms
            Column {
                visible: root.kind === "color" && root.item && root.item.swatch.a > 0
                width: parent.width
                spacing: 6
                Rectangle {
                    width: parent.width
                    height: 96
                    radius: 14
                    color: root.item ? root.item.swatch : "transparent"
                    border.width: 1
                    border.color: Qt.rgba(0, 0, 0, 0.12)
                }
                Repeater {
                    model: root.kind === "color" && root.item ? root.colorForms(root.item.swatch) : []
                    delegate: CopyLine { required property string modelData; value: modelData }
                }
            }

            // date: a calendar leaf beside the full date and how far away it is
            Row {
                visible: root.dateParts.length >= 3
                width: parent.width
                spacing: 14
                Rectangle {
                    width: 72
                    height: 76
                    radius: 16
                    color: colors.keycap
                    border.width: 1
                    border.color: colors.hairline
                    Column {
                        anchors.centerIn: parent
                        spacing: -2
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.dateParts.length ? root.dateParts[0].slice(0, 3) : ""
                            color: root.accent
                            font.pixelSize: theme.metaSize + 1
                            font.weight: Font.DemiBold
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.dateParts.length > 1 ? root.dateParts[1].split(" ")[0] : ""
                            color: colors.foreground
                            font.pixelSize: 34
                            font.weight: Font.Light
                        }
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 86
                    spacing: 4
                    Text {
                        width: parent.width
                        text: root.dateParts.length > 1 ? root.dateParts[0] + ", " + root.dateParts[1] : ""
                        color: colors.foreground
                        font.pixelSize: theme.bodySize + 1
                        font.weight: Font.Medium
                        wrapMode: Text.Wrap
                    }
                    Text {
                        width: parent.width
                        text: root.dateParts.length > 2 ? root.dateParts[2].charAt(0).toUpperCase() + root.dateParts[2].slice(1) : ""
                        color: colors.muted
                        font.pixelSize: theme.metaSize + 1
                    }
                }
            }

            // time: as copied, large, with the day it resolves to
            Column {
                visible: root.kind === "time"
                width: parent.width
                spacing: 4
                Text {
                    text: root.text.trim()
                    color: colors.foreground
                    font.pixelSize: 34
                    font.weight: Font.Light
                    font.features: { "tnum": 1 }
                }
                Text {
                    width: parent.width
                    text: root.item ? (root.item.whenLabel.length > 0 ? root.item.whenLabel : root.item.detail) : ""
                    color: colors.muted
                    font.pixelSize: theme.metaSize + 1
                    wrapMode: Text.Wrap
                }
            }

            // link: the host large, the path beneath
            Column {
                visible: root.kind === "link"
                width: parent.width
                spacing: 6
                Text {
                    width: parent.width
                    text: root.item ? root.item.title : ""
                    color: colors.foreground
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    wrapMode: Text.WrapAnywhere
                }
                Text {
                    width: parent.width
                    visible: text.length > 0
                    text: root.item ? root.item.titleTail : ""
                    color: colors.muted
                    font.pixelSize: theme.metaSize + 1
                    wrapMode: Text.WrapAnywhere
                }
            }

            // possible password: hidden until the user chooses to show it
            Column {
                visible: root.kind === "secret"
                width: parent.width
                spacing: 8
                Rectangle {
                    width: parent.width
                    height: secretText.implicitHeight + 24
                    radius: 12
                    color: colors.keycap
                    Text {
                        id: secretText
                        anchors.left: parent.left
                        anchors.right: revealToggle.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8
                        text: root.revealed ? root.text : (root.item ? root.item.title : "")
                        color: colors.foreground
                        font.family: theme.monoFamily
                        font.pixelSize: theme.codeSize + 1
                        font.letterSpacing: root.revealed ? 0 : 2
                        wrapMode: Text.WrapAnywhere
                    }
                    Text {
                        id: revealToggle
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.revealed ? "Hide" : "Show"
                        color: revealArea.containsMouse ? colors.foreground : colors.muted
                        font.pixelSize: theme.metaSize
                        font.weight: Font.Medium
                        MouseArea {
                            id: revealArea
                            anchors.fill: parent
                            anchors.margins: -6
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.revealed = !root.revealed
                        }
                    }
                }
                Text {
                    width: parent.width
                    text: "This looks like a password or key, so it stays hidden everywhere in the app until you show it."
                    color: colors.muted
                    font.pixelSize: theme.metaSize
                    wrapMode: Text.Wrap
                }
            }

            // code: as written, never wrapped, filling the space the pane has
            CodeView {
                id: codeView
                visible: root.kind === "code"
                width: parent.width
                height: visible ? implicitHeight : 0
                code: visible ? root.text : ""
                language: root.item ? root.item.kindLabel : ""
                maxHeight: Math.max(120, scroller.height - headerRow.height - factsLine.implicitHeight
                                         - (kindReading.visible ? kindReading.implicitHeight + body.spacing : 0)
                                         - body.spacing * 2)
            }

            // paths and verification codes: monospace, as written
            Rectangle {
                visible: root.kind === "path" || root.kind === "verification"
                width: parent.width
                height: Math.min(codeText.implicitHeight + 24, 220)
                radius: 12
                color: colors.keycap
                clip: true
                Text {
                    id: codeText
                    anchors.fill: parent
                    anchors.margins: 12
                    text: root.text
                    color: colors.foreground
                    font.family: theme.monoFamily
                    font.pixelSize: root.kind === "verification" ? 26 : theme.codeSize
                    font.letterSpacing: root.kind === "verification" ? 4 : 0
                    horizontalAlignment: root.kind === "verification" ? Text.AlignHCenter : Text.AlignLeft
                    wrapMode: Text.WrapAnywhere
                }
            }

            // everything else: the full text
            Text {
                visible: ["color", "date", "time", "link", "secret", "code", "path", "verification"].indexOf(root.kind) < 0
                         || (root.kind === "color" && root.item && root.item.swatch.a === 0)
                         || (root.kind === "date" && root.dateParts.length < 3)
                width: parent.width
                text: root.showingTranslation ? root.translation : root.text
                color: colors.foreground
                font.pixelSize: theme.bodySize
                lineHeight: 1.3
                wrapMode: Text.Wrap
                maximumLineCount: 14
                elide: Text.ElideRight
            }

            Text {
                visible: root.showingTranslation
                width: parent.width
                text: "Translated from " + root.translatableFrom + " on this device"
                color: colors.muted
                font.pixelSize: theme.metaSize
            }

            // kind-specific reading, when it adds something the body does not
            Text {
                id: kindReading
                visible: root.item !== null && ["text", "code", "color", "date", "time", "link", "secret"].indexOf(root.kind) < 0
                         && root.item.detail !== root.item.kindLabel
                width: parent.width
                text: root.item ? root.item.detail : ""
                color: colors.muted
                font.pixelSize: theme.metaSize + 1
                wrapMode: Text.Wrap
            }

            Text {
                id: factsLine
                visible: root.kind !== "secret"
                width: parent.width
                // a long paste is previewed from its start, and says so
                text: root.facts(root.text) + (root.kind === "code" && codeView.shownLines < codeView.lines
                                               ? "   showing the first " + codeView.shownLines : "")
                color: colors.muted
                font.pixelSize: theme.metaSize
                font.features: { "tnum": 1 }
            }
        }
    }

    // ---- actions: one aligned list, pinned to the bottom ----
    Column {
        id: actionArea
        visible: root.item !== null
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 8
        spacing: 0

        // the outcome of the last request, just above the actions it came from
        Text {
            width: parent.width
            leftPadding: 10
            rightPadding: 10
            bottomPadding: 6
            visible: root.status.length > 0
            text: (root.statusOk ? "✓ " : "") + root.status
            color: root.statusOk ? colors.foreground : colors.blocked
            font.pixelSize: theme.metaSize
            wrapMode: Text.Wrap
        }

        Item {
            width: parent.width
            height: 9
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                height: 1
                color: colors.hairline
            }
        }

        Repeater {
            model: root.actions
            delegate: Column {
                id: entry
                required property var modelData
                required property int index
                readonly property bool open: root.composing === modelData.id
                width: actionArea.width

                Rectangle {
                    id: actionRow
                    width: parent.width
                    height: 30
                    radius: 8
                    color: rowArea.containsMouse || entry.open ? colors.hover : "transparent"
                    scale: rowArea.pressed ? 0.98 : 1
                    Behavior on color { ColorAnimation { duration: theme.hoverMs } }
                    Behavior on scale {
                        enabled: !root.reduceMotion
                        SpringAnimation { spring: 9; damping: 0.45; epsilon: 0.002 }
                    }

                    KindGlyph {
                        id: actionGlyph
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        kind: entry.modelData.glyph
                        tint: entry.index === 0 ? colors.foreground : colors.muted
                    }
                    Text {
                        anchors.left: actionGlyph.right
                        anchors.leftMargin: 10
                        anchors.right: shortcut.left
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        text: entry.modelData.label
                        color: colors.foreground
                        font.pixelSize: theme.metaSize + 1
                        font.weight: entry.index === 0 ? Font.Medium : Font.Normal
                        elide: Text.ElideRight
                    }
                    Text {
                        id: shortcut
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: entry.modelData.key
                        color: colors.muted
                        font.pixelSize: theme.metaSize
                    }
                    MouseArea {
                        id: rowArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.trigger(entry.modelData, actionRow)
                    }
                }

                // the title field, opened under the action that needs it
                Item {
                    width: parent.width
                    height: entry.open ? 40 : 0
                    clip: true
                    Behavior on height {
                        enabled: !root.reduceMotion
                        NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
                    }
                    onHeightChanged: {
                        if (entry.open && height > 0 && !titleInput.activeFocus) {
                            titleInput.forceActiveFocus();
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 36
                        anchors.rightMargin: 4
                        anchors.topMargin: 4
                        anchors.bottomMargin: 6
                        radius: 8
                        color: colors.keycap
                        border.width: titleInput.activeFocus ? 1 : 0
                        border.color: root.accent

                        TextInput {
                            id: titleInput
                            anchors.left: parent.left
                            anchors.right: hint.left
                            anchors.leftMargin: 10
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            color: colors.foreground
                            selectionColor: root.accent
                            selectedTextColor: colors.accentForeground
                            font.pixelSize: theme.metaSize + 1
                            selectByMouse: true
                            clip: true
                            Keys.onReturnPressed: root.confirm(entry.modelData.id, text)
                            Keys.onEnterPressed: root.confirm(entry.modelData.id, text)
                            Keys.onEscapePressed: {
                                root.composing = "";
                                root.doneEditing();
                            }
                            onVisibleChanged: if (!visible) text = ""
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.NoButton
                                cursorShape: Qt.IBeamCursor
                            }
                        }
                        Text {
                            anchors.fill: titleInput
                            verticalAlignment: Text.AlignVCenter
                            visible: titleInput.text.length === 0
                            text: "What is it for?"
                            color: colors.muted
                            font.pixelSize: theme.metaSize + 1
                        }
                        Text {
                            id: hint
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: platformUi.keyText("Return")
                            color: colors.muted
                            font.pixelSize: theme.metaSize
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -6
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.confirm(entry.modelData.id, titleInput.text)
                            }
                        }
                    }
                }
            }
        }
    }
}
