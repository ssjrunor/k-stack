/*
  Author: Runor Ewhro
  Description: shows the session clipboard history as a keyboard-first panel
               that springs open over the current app from the tray control, with category filters, grouping, a preview pane, and
               a suggested action for each kind of entry.
*/
import QtQuick
import QtQuick.Window
import "components" as Components
import "theme" as Style

Window {
    id: window
    width: theme.panelWidth + (viewOptions.previewVisible ? theme.previewWidth : 0)
    height: theme.searchHeight + theme.filterHeight + theme.listPadding * 2
            + theme.rowHeight * theme.visibleRows + theme.footerHeight
    visible: false
    title: "Clipboard history"
    // without a tray icon this window is the only way in, so it
    // stays an ordinary window that never hides itself
    flags: summonMode ? Qt.Tool | Qt.FramelessWindowHint : Qt.Window
    color: summonMode ? "transparent" : colors.panel

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    // with a system material behind the window, QML draws only content and
    // leaves the surface, edge, and shadow to the platform
    readonly property bool nativeMaterial: platformUi.nativeMaterial
    readonly property real cornerRadius: theme.panelRadius
    readonly property date sessionStart: new Date()
    readonly property bool capturing: settingsController.captureEnabled
    readonly property bool blocked: settingsController.captureBlocked
    readonly property bool searching: searchModel.query.length > 0
    readonly property bool detailed: viewOptions.density === "detailed"

    // one opacity and scale drive both the QML content and the native glass,
    // so the two move as a single surface
    property real revealOpacity: 1
    property real revealScale: 1
    property bool reduceMotion: false
    property bool dismissing: false
    property bool selectionSprings: false
    property color accent: colors.accent
    // the panel stays centred on this point as the preview widens it
    property real centerX: 0

    Binding {
        target: window
        property: "x"
        value: window.centerX - window.width / 2
        when: summonMode && window.centerX > 0
    }
    Binding {
        target: searchModel
        property: "groupBy"
        value: viewOptions.groupBy
    }

    Behavior on width {
        enabled: window.visible && !window.reduceMotion
        NumberAnimation { duration: 240; easing.type: Easing.OutCubic }
    }

    onRevealOpacityChanged: syncMaterial()
    onRevealScaleChanged: syncMaterial()

    function syncMaterial() {
        platformUi.setReveal(revealOpacity, revealScale);
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
        // copy what the preview shows, so a translated entry copies its
        // translation; switching back to the original copies the original
        if (preview.showingTranslation && preview.item === row) {
            historyController.copyText(preview.translation);
        } else {
            historyController.copy(row.entryId);
        }
        row.copied = true;
        closeTimer.start();
    }

    function removeSelected() {
        if (results.currentItem) {
            results.currentItem.collapseAndRemove();
        }
    }

    // share the selected entry from a point in window coordinates; links and
    // paths share as links and files, possible passwords never share
    function shareSelected(x, y) {
        const row = results.currentItem;
        if (!systemServices.canShare || !row || row.sensitive || results.count === 0) {
            return;
        }
        const url = row.kind === "link" || row.kind === "path" ? row.actionUrl : "";
        if (x === undefined) {
            const point = row.mapToItem(null, row.width - 60, row.height);
            x = point.x;
            y = point.y;
        }
        systemServices.share(row.contentText, url, x, y);
    }

    // run the selected entry's suggested action: open it, show it on a map,
    // copy a converted form of it, or remove it
    function performAction() {
        const row = results.currentItem;
        if (!row || row.actionLabel.length === 0 || dismissing) {
            return;
        }
        if (row.actionKind === "remove") {
            row.collapseAndRemove();
        } else if (row.actionKind === "open") {
            Qt.openUrlExternally(row.actionUrl);
            dismiss();
        } else if (row.actionKind === "map") {
            systemServices.showOnMap(row.actionText);
            dismiss();
        } else if (row.actionKind === "copy") {
            historyController.copyText(row.actionText);
            row.copied = true;
            closeTimer.start();
        }
    }

    function togglePreview() {
        viewOptions.previewVisible = !viewOptions.previewVisible;
    }

    function dismiss() {
        if (!summonMode || dismissing || !visible) {
            return;
        }
        dismissing = true;
        openAnimation.stop();
        closeAnimation.start();
    }

    // open centred in the top third of the screen with a fresh search and
    // filter, the way a summoned panel should, then hand keys to the input
    onVisibleChanged: {
        if (!visible || !summonMode) {
            return;
        }
        reduceMotion = platformUi.reduceMotion();
        accent = platformUi.accentColor();
        systemServices.resetBusy();
        selectionSprings = false;
        revealOpacity = 0;
        revealScale = reduceMotion ? 1 : theme.openFromScale;
        centerX = Screen.virtualX + Screen.width / 2;
        y = Screen.virtualY + Screen.height * 0.2;
        search.text = "";
        searchModel.category = "";
        searchModel.refresh();
        select(0);
        search.input.forceActiveFocus();
        openAnimation.start();
        Qt.callLater(() => window.selectionSprings = !window.reduceMotion);
    }
    // a share sheet, permission prompt, or calendar request can take focus
    // briefly; the panel stays until it finishes
    onActiveChanged: {
        if (!active && !closeTimer.running && !viewOptions.menuOpen && !systemServices.busy) {
            dismiss();
        }
    }

    Connections {
        target: systemServices
        function onBusyChanged() {
            if (!systemServices.busy && !window.active && window.visible && !closeTimer.running) {
                window.dismiss();
            }
        }
        function onFinished(message, succeeded) {
            preview.status = message;
            preview.statusOk = succeeded;
            // an outcome worth seeing: bring the panel back if a permission
            // prompt took focus while the request ran
            if (window.visible && !window.active) {
                window.requestActivate();
            }
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
                placeholder: searchModel.matchCount === 0 || window.searching ? "Search"
                             : "Search " + searchModel.matchCount + (searchModel.matchCount === 1 ? " item" : " items")
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
                    } else if (event.key === Qt.Key_Tab) {
                        filter.step(1);
                    } else if (event.key === Qt.Key_Backtab) {
                        filter.step(-1);
                    } else if (event.key === Qt.Key_Space && search.text.length === 0) {
                        window.togglePreview();
                    } else if (event.key === Qt.Key_Escape) {
                        if (search.text.length > 0) {
                            search.text = "";
                        } else if (searchModel.category.length > 0) {
                            searchModel.category = "";
                        } else {
                            window.dismiss();
                        }
                    } else if (command && event.key === Qt.Key_Backspace) {
                        window.removeSelected();
                    } else if (command && (event.modifiers & Qt.ShiftModifier) && event.key === Qt.Key_S) {
                        window.shareSelected();
                    } else if (command && event.key === Qt.Key_O) {
                        window.performAction();
                    } else if (command && event.key === Qt.Key_Y) {
                        window.togglePreview();
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
                anchors.right: more.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                capturing: window.capturing
                blocked: window.blocked
                label: settingsController.captureStatus
            }

            // view options open as a native menu below this button
            Rectangle {
                id: more
                anchors.right: parent.right
                anchors.rightMargin: theme.sideMargin - 6
                anchors.verticalCenter: parent.verticalCenter
                width: 28
                height: 28
                radius: 14
                color: moreArea.containsMouse || viewOptions.menuOpen ? colors.keycap : "transparent"
                scale: moreArea.pressed ? 0.92 : 1
                Behavior on color { ColorAnimation { duration: theme.hoverMs } }
                Behavior on scale {
                    enabled: !window.reduceMotion
                    SpringAnimation { spring: 9; damping: 0.4; epsilon: 0.002 }
                }
                Row {
                    anchors.centerIn: parent
                    spacing: 3
                    Repeater {
                        model: 3
                        Rectangle { width: 3.5; height: 3.5; radius: 1.75; color: colors.muted }
                    }
                }
                MouseArea {
                    id: moreArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        const point = more.mapToGlobal(0, more.height + 6);
                        viewOptions.showMenu(point.x, point.y);
                        search.input.forceActiveFocus();
                    }
                }
            }
        }

        Components.CategoryFilter {
            id: filter
            anchors.top: header.bottom
            width: parent.width
            categories: searchModel.categories
            total: searchModel.matchCount
            current: searchModel.category
            reduceMotion: window.reduceMotion
            visible: searchModel.matchCount > 0
            onChosen: (id) => {
                searchModel.category = id;
                window.select(0);
            }
        }

        Rectangle {
            id: divider
            anchors.top: filter.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: theme.sideMargin
            anchors.rightMargin: theme.sideMargin
            height: 1
            color: colors.hairline
        }

        Item {
            id: listArea
            anchors.top: divider.bottom
            anchors.bottom: footer.top
            anchors.left: parent.left
            width: theme.panelWidth

            ListView {
                id: results
                anchors.fill: parent
                anchors.margins: theme.listPadding
                clip: true
                model: searchModel
                boundsBehavior: Flickable.StopAtBounds
                onCountChanged: {
                    if (currentIndex < 0 && count > 0) {
                        currentIndex = 0;
                    }
                }

                section.property: "section"
                section.criteria: ViewSection.FullString
                section.delegate: Item {
                    required property string section
                    width: results.width
                    height: section.length > 0 ? theme.sectionHeight : 0
                    visible: section.length > 0
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 6
                        text: parent.section
                        color: colors.muted
                        font.pixelSize: theme.metaSize
                        font.weight: Font.DemiBold
                    }
                }

                // the selection is one shared shape that glides between rows on
                // a spring and squeezes with the row under the pointer
                highlightFollowsCurrentItem: false
                highlight: Rectangle {
                    width: results.width
                    height: results.currentItem ? results.currentItem.height : theme.rowHeight
                    y: results.currentItem ? results.currentItem.y : 0
                    radius: theme.rowRadius
                    color: window.accent
                    visible: results.count > 0 && results.currentItem !== null
                    scale: results.currentItem && results.currentItem.pressed ? 0.985 : 1
                    Behavior on y {
                        enabled: window.selectionSprings
                        SpringAnimation { spring: 6; damping: 0.5; epsilon: 0.25 }
                    }
                    Behavior on height {
                        enabled: window.selectionSprings
                        NumberAnimation { duration: theme.hoverMs }
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
                    detailed: window.detailed
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
        }

        Components.PreviewPane {
            id: preview
            anchors.top: divider.bottom
            anchors.bottom: footer.top
            anchors.left: listArea.right
            width: theme.previewWidth
            item: results.count > 0 ? results.currentItem : null
            accent: window.accent
            reduceMotion: window.reduceMotion
            opacity: viewOptions.previewVisible ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: theme.fadeInMs } }
            onCopyRequested: window.copyAt(results.currentIndex)
            onActionRequested: window.performAction()
            canShare: systemServices.canShare
            canAddEvent: systemServices.canAddEvent
            canShowCalendarDay: systemServices.canShowCalendarDay
            canAddReminder: systemServices.canAddReminder
            onShareRequested: (x, y) => window.shareSelected(x, y)
            onEventRequested: (title) => {
                if (results.currentItem) {
                    systemServices.addCalendarEvent(title, results.currentItem.when, results.currentItem.allDay);
                }
            }
            onReminderRequested: (title) => {
                if (results.currentItem) {
                    systemServices.addReminder(title, results.currentItem.when, results.currentItem.allDay);
                }
            }
            onCalendarDayRequested: {
                if (results.currentItem) {
                    systemServices.showCalendarDay(results.currentItem.when);
                }
            }
            onDoneEditing: search.input.forceActiveFocus()
            onCopyTextRequested: (text) => {
                historyController.copyText(text);
                if (results.currentItem) {
                    results.currentItem.copied = true;
                }
                closeTimer.start();
            }
        }

        Components.PanelFooter {
            id: footer
            anchors.bottom: parent.bottom
            width: parent.width
            capturing: window.capturing
            hasItems: results.count > 0
            previewVisible: viewOptions.previewVisible
            actionLabel: results.currentItem && results.count > 0 ? results.currentItem.actionLabel : ""
            reduceMotion: window.reduceMotion
            note: settingsController.persistentHistory
                  ? "Saved on this device"
                  : "Session only · clears on quit"
            onCopyRequested: window.copyAt(results.currentIndex)
            onActionRequested: window.performAction()
            onPreviewRequested: window.togglePreview()
            onPauseRequested: settingsController.captureEnabled = !window.capturing
        }
    }
}
