/*
 * Author: Runor Ewhro
 * Description: draws the small line glyphs used across the panel: one per
 *              entry kind, telling kinds apart by shape, and one per action
 *              in the preview's action list; a color entry shows a real
 *              swatch instead, so the content's own color is the marker.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    property string kind: "text"
    property color swatch: "transparent"
    property color tint: colors.muted
    implicitWidth: theme.glyphSize
    implicitHeight: theme.glyphSize

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    readonly property bool showsSwatch: kind === "color" && swatch.a > 0

    Rectangle {
        visible: root.showsSwatch
        anchors.centerIn: parent
        width: 14
        height: 14
        radius: 4
        color: root.swatch
        border.width: 1
        border.color: Qt.rgba(0, 0, 0, 0.18)
    }

    Canvas {
        id: canvas
        visible: !root.showsSwatch
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            ctx.strokeStyle = root.tint;
            ctx.fillStyle = root.tint;
            ctx.lineWidth = 1.4;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";
            const line = (x1, y1, x2, y2) => { ctx.moveTo(x1, y1); ctx.lineTo(x2, y2); };
            const rounded = (x, y, w, h, r) => {
                ctx.moveTo(x + r, y);
                ctx.arcTo(x + w, y, x + w, y + h, r);
                ctx.arcTo(x + w, y + h, x, y + h, r);
                ctx.arcTo(x, y + h, x, y, r);
                ctx.arcTo(x, y, x + w, y, r);
                ctx.closePath();
            };
            ctx.beginPath();
            switch (root.kind) {
            case "link":
                ctx.translate(8, 8);
                ctx.rotate(-Math.PI / 4);
                rounded(-6.5, -2.4, 7.5, 4.8, 2.4);
                rounded(-1, -2.4, 7.5, 4.8, 2.4);
                break;
            case "code":
                line(5.5, 4, 2, 8); line(2, 8, 5.5, 12);
                line(10.5, 4, 14, 8); line(14, 8, 10.5, 12);
                break;
            case "path":
                ctx.moveTo(2, 4); ctx.lineTo(6.5, 4); ctx.lineTo(8, 5.5); ctx.lineTo(14, 5.5);
                ctx.lineTo(14, 12.5); ctx.lineTo(2, 12.5); ctx.closePath();
                break;
            case "email":
                rounded(2, 4, 12, 8.5, 1.5);
                ctx.moveTo(2.5, 4.5); ctx.lineTo(8, 9); ctx.lineTo(13.5, 4.5);
                break;
            case "phone":
                rounded(4.5, 1.8, 7, 12.4, 1.8);
                line(7, 12, 9, 12);
                break;
            case "address":
                ctx.moveTo(8, 14.5);
                ctx.bezierCurveTo(4.5, 10.5, 3.5, 8.5, 3.5, 6.5);
                ctx.arc(8, 6.5, 4.5, Math.PI, 0, false);
                ctx.bezierCurveTo(12.5, 8.5, 11.5, 10.5, 8, 14.5);
                ctx.moveTo(9.6, 6.5);
                ctx.arc(8, 6.5, 1.6, 0, Math.PI * 2);
                break;
            case "number":
                line(6, 3, 5, 13); line(11, 3, 10, 13);
                line(3, 6.5, 13.5, 6.5); line(2.5, 9.5, 13, 9.5);
                break;
            case "verification":
                ctx.moveTo(8, 1.8); ctx.lineTo(13.5, 3.8); ctx.lineTo(13.5, 8);
                ctx.bezierCurveTo(13.5, 11, 11, 13.2, 8, 14.4);
                ctx.bezierCurveTo(5, 13.2, 2.5, 11, 2.5, 8);
                ctx.lineTo(2.5, 3.8); ctx.closePath();
                ctx.moveTo(5.5, 8); ctx.lineTo(7.4, 9.9); ctx.lineTo(10.8, 6.2);
                break;
            case "date":
                rounded(2.5, 3.5, 11, 10, 2);
                line(2.5, 6.5, 13.5, 6.5); line(5.5, 2, 5.5, 4.5); line(10.5, 2, 10.5, 4.5);
                break;
            case "time":
                ctx.moveTo(14, 8);
                ctx.arc(8, 8, 6, 0, Math.PI * 2);
                ctx.moveTo(8, 4.5); ctx.lineTo(8, 8); ctx.lineTo(10.5, 9.5);
                break;
            case "secret":
                rounded(3, 7, 10, 7, 1.5);
                ctx.moveTo(5, 7); ctx.lineTo(5, 5.5);
                ctx.arc(8, 5.5, 3, Math.PI, 0, false);
                ctx.lineTo(11, 7);
                break;
            case "color":
                ctx.moveTo(14, 8);
                ctx.arc(8, 8, 6, 0, Math.PI * 2);
                break;
            case "action-copy":
                rounded(5.5, 5.5, 8, 8.5, 2);
                ctx.moveTo(10.5, 3.5); ctx.lineTo(10.5, 3.2);
                ctx.arcTo(10.5, 2, 9.3, 2, 1.2); ctx.lineTo(3.7, 2);
                ctx.arcTo(2.5, 2, 2.5, 3.2, 1.2); ctx.lineTo(2.5, 9.3);
                ctx.arcTo(2.5, 10.5, 3.7, 10.5, 1.2); ctx.lineTo(4, 10.5);
                break;
            case "action-open":
                ctx.moveTo(7, 3); ctx.lineTo(4, 3);
                ctx.arcTo(2.5, 3, 2.5, 4.5, 1.5); ctx.lineTo(2.5, 12);
                ctx.arcTo(2.5, 13.5, 4, 13.5, 1.5); ctx.lineTo(11.5, 13.5);
                ctx.arcTo(13, 13.5, 13, 12, 1.5); ctx.lineTo(13, 9);
                line(8, 8, 13.5, 2.5); line(9.5, 2.5, 13.5, 2.5); line(13.5, 2.5, 13.5, 6.5);
                break;
            case "action-share":
                ctx.moveTo(5.5, 6.5); ctx.lineTo(4, 6.5);
                ctx.arcTo(3, 6.5, 3, 7.5, 1); ctx.lineTo(3, 13);
                ctx.arcTo(3, 14, 4, 14, 1); ctx.lineTo(12, 14);
                ctx.arcTo(13, 14, 13, 13, 1); ctx.lineTo(13, 7.5);
                ctx.arcTo(13, 6.5, 12, 6.5, 1); ctx.lineTo(10.5, 6.5);
                line(8, 9.5, 8, 1.8); line(5.5, 4.2, 8, 1.8); line(8, 1.8, 10.5, 4.2);
                break;
            case "action-reminder":
                ctx.moveTo(14, 8);
                ctx.arc(8, 8, 6, 0, Math.PI * 2);
                ctx.moveTo(5.5, 8); ctx.lineTo(7.3, 9.8); ctx.lineTo(10.6, 6.2);
                break;
            case "action-day":
                rounded(2.5, 3.5, 11, 10, 2);
                line(2.5, 6.5, 13.5, 6.5); line(5.5, 2, 5.5, 4.5); line(10.5, 2, 10.5, 4.5);
                ctx.moveTo(9.2, 10);
                ctx.arc(8, 10, 1.2, 0, Math.PI * 2);
                break;
            case "action-translate":
                // a character beside a letter: the usual sign for translation
                line(2, 4, 9.5, 4); line(5.75, 2.2, 5.75, 4);
                ctx.moveTo(8, 4); ctx.quadraticCurveTo(7, 8.5, 2.5, 11);
                ctx.moveTo(4, 6); ctx.quadraticCurveTo(5.5, 8.5, 8.5, 10);
                ctx.moveTo(8.5, 14.5); ctx.lineTo(11.5, 7); ctx.lineTo(14.5, 14.5);
                line(9.7, 11.8, 13.3, 11.8);
                break;
            case "action-remove":
                line(2.5, 4.5, 13.5, 4.5);
                ctx.moveTo(6, 4.5); ctx.lineTo(6, 3); ctx.lineTo(10, 3); ctx.lineTo(10, 4.5);
                ctx.moveTo(4, 4.5); ctx.lineTo(4.8, 13.5); ctx.lineTo(11.2, 13.5); ctx.lineTo(12, 4.5);
                break;
            default:
                line(3, 4.5, 13, 4.5); line(3, 8, 13, 8); line(3, 11.5, 9, 11.5);
            }
            ctx.stroke();
        }
    }

    onKindChanged: canvas.requestPaint()
    onTintChanged: canvas.requestPaint()
}
