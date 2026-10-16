/*
 * Author: Runor Ewhro
 * Description: tells the user what to do when the list is empty: copy
 *              something, change the search, or restore clipboard access.
 */
import QtQuick
import "../theme" as Style

Column {
    id: root
    property bool searching: false
    property bool blocked: false
    property string query: ""
    signal retryRequested()
    spacing: 6
    width: 360

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    Text {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: root.blocked ? "Clipboard capture unavailable"
              : root.searching ? "No items match “" + root.query + "”."
              : "Nothing copied yet"
        color: colors.foreground
        font.pixelSize: theme.bodySize
        font.weight: Font.Medium
    }
    Text {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        visible: !root.searching || root.blocked
        text: root.blocked ? "Check clipboard access, then choose Try again."
                           : "Copy text or a link in any app and it shows up here."
        color: colors.muted
        font.pixelSize: theme.metaSize + 1
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: root.blocked
        text: "Try again"
        color: colors.accent
        font.pixelSize: theme.metaSize + 1
        font.weight: Font.Medium
        topPadding: 4
        MouseArea {
            anchors.fill: parent
            anchors.margins: -6
            cursorShape: Qt.PointingHandCursor
            onClicked: root.retryRequested()
        }
    }
}
