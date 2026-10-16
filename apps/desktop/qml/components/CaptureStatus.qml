/*
 * Author: Runor Ewhro
 * Description: shows whether clipboard capture is running, paused, or
 *              blocked, as a small dot and word beside the search input.
 */
import QtQuick
import "../theme" as Style

Row {
    id: root
    property bool capturing: false
    property bool blocked: false
    property string label: ""
    spacing: 6

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    Rectangle {
        width: 7
        height: 7
        radius: 3.5
        anchors.verticalCenter: parent.verticalCenter
        color: root.capturing ? colors.live : root.blocked ? colors.blocked : "transparent"
        border.width: root.capturing || root.blocked ? 0 : 1.5
        border.color: colors.muted
        Behavior on color { ColorAnimation { duration: 220 } }
    }
    Text {
        text: root.label
        color: root.blocked ? colors.blocked : colors.muted
        font.pixelSize: theme.metaSize
        Behavior on color { ColorAnimation { duration: 220 } }
        anchors.verticalCenter: parent.verticalCenter
    }
}
