/*
 * Author: Runor Ewhro
 * Description: holds the sizes, type scale, and motion timings that shape
 *              the panel, so components stay aligned to one grid and
 *              move with one rhythm.
 */
import QtQml

QtObject {
    readonly property int panelWidth: 600
    readonly property int panelRadius: 22
    readonly property int searchHeight: 58
    readonly property int footerHeight: 34
    readonly property int listPadding: 8
    readonly property int rowHeight: 38
    readonly property int rowRadius: 12
    readonly property int visibleRows: 9
    readonly property int sideMargin: 20

    readonly property int searchSize: 22
    readonly property int bodySize: 14
    readonly property int codeSize: 13
    readonly property int metaSize: 12
    readonly property int keySize: 11
    readonly property string monoFamily: "Menlo"

    // motion: the panel springs open from slightly small with a soft
    // overshoot, and leaves faster than it arrives
    readonly property int copiedHoldMs: 260
    readonly property int openMs: 440
    readonly property int fadeInMs: 160
    readonly property int closeMs: 150
    readonly property int hoverMs: 120
    readonly property int collapseMs: 180
    readonly property real openFromScale: 0.94
    readonly property real closeToScale: 0.97
    readonly property real openOvershoot: 1.4
}
