/*
 * Author: Runor Ewhro
 * Description: draws the panel's search input with its magnifier and
 *              placeholder; the panel reads keys from the exposed input.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    property alias text: input.text
    property alias input: input
    property string placeholder: "Search"
    implicitHeight: theme.searchHeight

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    Canvas {
        id: glass
        width: 18
        height: 18
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        property color stroke: colors.muted
        onStrokeChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            ctx.strokeStyle = stroke;
            ctx.lineWidth = 1.6;
            ctx.lineCap = "round";
            ctx.beginPath();
            ctx.arc(7.5, 7.5, 5.5, 0, Math.PI * 2);
            ctx.moveTo(11.6, 11.6);
            ctx.lineTo(16, 16);
            ctx.stroke();
        }
    }

    TextInput {
        id: input
        anchors.left: glass.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        color: colors.foreground
        selectionColor: colors.accent
        selectedTextColor: colors.accentForeground
        font.pixelSize: theme.searchSize
        font.letterSpacing: -0.2
        selectByMouse: true
        clip: true
    }

    // show the text cursor over the whole input line without taking clicks
    MouseArea {
        anchors.fill: input
        acceptedButtons: Qt.NoButton
        cursorShape: Qt.IBeamCursor
    }

    Text {
        anchors.fill: input
        verticalAlignment: Text.AlignVCenter
        text: root.placeholder
        color: colors.muted
        font: input.font
        visible: input.text.length === 0
    }
}
