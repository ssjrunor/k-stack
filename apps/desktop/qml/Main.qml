/*
  Author: Runor Ewhro
  Description: shows the session clipboard history as a keyboard-first panel
               that springs open over the current app from the menu bar
               control, copies the chosen item, and gets out of the way.
*/
import QtQuick
import QtQuick.Window
import "components" as Components
import "theme" as Style

Window {
    id: window
    width: theme.panelWidth
    height: theme.searchHeight + theme.listPadding * 2 + theme.rowHeight * theme.visibleRows + theme.footerHeight
    visible: false
    title: "Clipboard history"
    // without a menu bar or tray icon this window is the only way in, so it
    // stays an ordinary window that never hides itself
    flags: summonMode ? Qt.Tool | Qt.FramelessWindowHint : Qt.Window
    color: summonMode ? "transparent" : colors.panel

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    // set by the app once the system glass sits behind this window; QML then
    // draws only content and leaves the surface, edge, and shadow to macOS
    property bool nativeMaterial: false
    property var materialBridge: null
    readonly property real cornerRadius: theme.panelRadius
    readonly property date sessionStart: new Date()
    readonly property bool capturing: settingsController.captureEnabled
    readonly property bool blocked: settingsController.captureBlocked
    readonly property bool searching: searchModel.query.length > 0

    // one opacity and scale drive both the QML content and the native glass,
    // so the two move as a single surface
    property real revealOpacity: 1
    property real revealScale: 1
    property bool reduceMotion: false
    property bool dismissing: false
    property bool selectionSprings: false

    onRevealOpacityChanged: syncMaterial()
    onRevealScaleChanged: syncMaterial()

    function syncMaterial() {
        if (materialBridge) {
            materialBridge.setReveal(revealOpacity, revealScale);
        }
    }

    function select(index) {
        if (results.count === 0) {
            return;
        }
        results.currentIndex = Math.max(0, Math.min(results.count - 1, index));
        results.positionViewAtIndex(results.currentIndex, ListView.Contain);
    }

    function copyAt(index) {
        if (index < 0 || index >= results.count || dismissing) {
            return;
        }
        select(index);
        const row = results.itemAtIndex(index);
        historyController.copy(row.entryId);
        row.copied = true;
        closeTimer.start();
    }

    function removeSelected() {
        if (results.currentItem) {
            results.currentItem.collapseAndRemove();
        }
    }

    function dismiss() {
        if (!summonMode || dismissing || !visible) {
            return;
        }
        dismissing = true;
        openAnimation.stop();
        closeAnimation.start();
    }

    // open centred in the top third of the screen with a fresh search, the way
    // a summoned panel should, then hand keys to the search input
    onVisibleChanged: {
        if (!visible || !summonMode) {
            return;
        }
        reduceMotion = materialBridge ? materialBridge.reduceMotion() : false;
        selectionSprings = false;
        revealOpacity = 0;
        revealScale = reduceMotion ? 1 : theme.openFromScale;
        x = Screen.virtualX + (Screen.width - width) / 2;
        y = Screen.virtualY + Screen.height * 0.22;
        search.text = "";
        select(0);
        search.input.forceActiveFocus();
        openAnimation.start();
        Qt.callLater(() => window.selectionSprings = !window.reduceMotion);
    }
    onActiveChanged: {
        if (!active && !closeTimer.running) {
            dismiss();
        }
    }

    ParallelAnimation {
        id: openAnimation
        NumberAnimation {
            target: window
            property: "revealOpacity"
            to: 1
            duration: theme.fadeInMs
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: window
            property: "revealScale"
            to: 1
            duration: window.reduceMotion ? 0 : theme.openMs
            easing.type: Easing.OutBack
            easing.overshoot: theme.openOvershoot
        }
    }

    SequentialAnimation {
        id: closeAnimation
        ParallelAnimation {
            NumberAnimation {
                target: window
                property: "revealOpacity"
                to: 0
                duration: theme.closeMs
                easing.type: Easing.InCubic
            }
            NumberAnimation {
                target: window
                property: "revealScale"
                to: window.reduceMotion ? 1 : theme.closeToScale
                duration: theme.closeMs
                easing.type: Easing.InCubic
            }
        }
        ScriptAction {
            script: {
                window.hide();
                window.dismissing = false;
            }
        }
    }

    Timer {
        id: closeTimer
        interval: theme.copiedHoldMs
        onTriggered: window.dismiss()
    }

    Rectangle {
        id: surface
        anchors.fill: parent
        radius: summonMode ? theme.panelRadius : 0
        color: window.nativeMaterial ? "transparent" : colors.panel
        border.width: summonMode && !window.nativeMaterial ? 1 : 0
        border.color: colors.panelEdge
        clip: true
        opacity: window.revealOpacity
        scale: window.revealScale

        Item {
            id: header
            width: parent.width
            height: theme.searchHeight

            Components.SearchField {
                id: search
                anchors.left: parent.left
                anchors.leftMargin: theme.sideMargin
                anchors.right: status.left
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                placeholder: results.count === 0 || window.searching ? "Search"
                             : "Search " + results.count + (results.count === 1 ? " item" : " items")
                onTextChanged: {
                    searchController.setQuery(text);
                    window.select(0);
                }

                input.Keys.onPressed: (event) => {
                    const command = event.modifiers & Qt.ControlModifier;
                    if (event.key === Qt.Key_Down) {
                        window.select(results.currentIndex + 1);
                    } else if (event.key === Qt.Key_Up) {
                        window.select(results.currentIndex - 1);
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        window.copyAt(results.currentIndex);
                    } else if (event.key === Qt.Key_Escape) {
                        if (search.text.length > 0) {
                            search.text = "";
                        } else {
                            window.dismiss();
                        }
                    } else if (command && event.key === Qt.Key_Backspace) {
                        window.removeSelected();
                    } else if (command && event.key === Qt.Key_P) {
                        settingsController.captureEnabled = !window.capturing;
                    } else if (command && event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                        window.copyAt(event.key - Qt.Key_1);
                    } else {
                        return;
                    }
                    event.accepted = true;
                }
            }

            Components.CaptureStatus {
                id: status
                anchors.right: parent.right
                anchors.rightMargin: theme.sideMargin
                anchors.verticalCenter: parent.verticalCenter
                capturing: window.capturing
                blocked: window.blocked
                label: settingsController.captureStatus
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: theme.sideMargin
                anchors.rightMargin: theme.sideMargin
                height: 1
                color: colors.hairline
            }
        }

        ListView {
            id: results
            anchors.top: header.bottom
            anchors.bottom: footer.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: theme.listPadding
            clip: true
            model: searchModel
            boundsBehavior: Flickable.StopAtBounds
            onCountChanged: {
                if (currentIndex < 0 && count > 0) {
                    currentIndex = 0;
                }
            }

            // the selection is one shared shape that glides between rows on a
            // spring and squeezes with the row under the pointer
            highlightFollowsCurrentItem: false
            highlight: Rectangle {
                width: results.width
                height: results.currentItem ? results.currentItem.height : theme.rowHeight
                y: results.currentItem ? results.currentItem.y : 0
                radius: theme.rowRadius
                color: colors.accent
                visible: results.count > 0 && results.currentItem !== null
                scale: results.currentItem && results.currentItem.pressed ? 0.985 : 1
                Behavior on y {
                    enabled: window.selectionSprings
                    SpringAnimation { spring: 6; damping: 0.5; epsilon: 0.25 }
                }
                Behavior on scale {
                    enabled: !window.reduceMotion
                    SpringAnimation { spring: 8; damping: 0.45; epsilon: 0.002 }
                }
            }

            delegate: Components.ClipboardItem {
                required property int index
                width: results.width
                selected: ListView.isCurrentItem
                reduceMotion: window.reduceMotion
                onHovered: results.currentIndex = index
                onCopyRequested: window.copyAt(index)
                onRemoveRequested: {
                    historyController.remove(entryId);
                    window.select(index);
                }
            }
        }

        Components.EmptyState {
            anchors.centerIn: results
            opacity: results.count === 0 ? 1 : 0
            visible: opacity > 0
            blocked: window.blocked
            searching: window.searching
            query: searchModel.query
            onRetryRequested: settingsController.captureEnabled = true
            Behavior on opacity { NumberAnimation { duration: theme.fadeInMs } }
        }

        Components.PanelFooter {
            id: footer
            anchors.bottom: parent.bottom
            width: parent.width
            capturing: window.capturing
            hasItems: results.count > 0
            note: results.count >= 90 && !window.searching
                  ? "Oldest items drop off after 100."
                  : "Session started " + window.sessionStart.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
            onCopyRequested: window.copyAt(results.currentIndex)
            onRemoveRequested: window.removeSelected()
            onPauseRequested: settingsController.captureEnabled = !window.capturing
        }
    }
}
