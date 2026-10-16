/*
 * Author: Runor Ewhro
 * Description: shows copied code as written: never wrapped, indentation kept,
 *              coloured by structure and value, with line numbers that stay
 *              in place while long lines scroll sideways. A soft edge marks
 *              each side where more code runs on.
 */
import QtQuick
import "../theme" as Style

Item {
    id: root
    property string code: ""
    // the entry's label: a language such as "Python", or "Code"
    property string language: ""
    // the tallest the view may grow before it scrolls
    property real maxHeight: 400
    readonly property int padding: 12
    readonly property var rendered: codeHighlighter.render(root.code, root.language, {
        "font": theme.monoFamily,
        "structure": colors.codeStructure,
        "value": colors.codeValue,
        "comment": colors.codeComment,
        "added": colors.codeAdded,
        "removed": colors.codeRemoved
    })
    readonly property int lines: rendered.lines
    readonly property int shownLines: rendered.shownLines

    implicitHeight: Math.min(codeText.implicitHeight + padding * 2, maxHeight)
    clip: true

    Accessible.role: Accessible.StaticText
    Accessible.name: (language === "Code" ? "Code" : language + " code") + ", " + lines
                     + (lines === 1 ? " line" : " lines")

    Style.Colors { id: colors }
    Style.Theme { id: theme }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: colors.keycap
    }

    Text {
        id: gutter
        x: root.padding
        y: root.padding - flick.contentY
        text: root.rendered.gutter
        textFormat: Text.RichText
        color: colors.muted
        opacity: 0.55
        font.family: theme.monoFamily
        font.pixelSize: theme.codeSize
        font.features: { "tnum": 1 }
    }

    Rectangle {
        id: rule
        x: gutter.x + gutter.implicitWidth + 10
        width: 1
        height: parent.height
        color: colors.hairline
    }

    Flickable {
        id: flick
        anchors.left: rule.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentWidth: codeText.implicitWidth + root.padding * 2
        contentHeight: codeText.implicitHeight + root.padding * 2
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.AutoFlickIfNeeded

        Text {
            id: codeText
            x: root.padding
            y: root.padding
            text: root.rendered.html
            textFormat: Text.RichText
            color: colors.foreground
            font.family: theme.monoFamily
            font.pixelSize: theme.codeSize
            wrapMode: Text.NoWrap
        }
    }

    // edges where code continues out of view
    Rectangle {
        anchors.left: flick.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 14
        visible: flick.contentX > 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: colors.codeEdge }
            GradientStop { position: 1; color: "transparent" }
        }
    }
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 14
        radius: 12
        visible: flick.contentX + flick.width < flick.contentWidth - 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: 1; color: colors.codeEdge }
        }
    }
}
