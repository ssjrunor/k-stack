/*
 * Author: Runor Ewhro
 * Description: turns a captured entry and its recognised kind into what the
 *              panel and menu show: a title, a useful detail, a category for
 *              filtering, and the one action that fits that kind of content.
 */
#pragma once

#include <kstack/core/ClipboardEntry.hpp>

#include <QColor>
#include <QDateTime>
#include <QString>
#include <QVector>

namespace kstack::desktop {

struct EntryCategory {
    QString id;
    QString label;
};

struct EntryPresentation {
    QString kind;          // stable id used by QML glyphs, e.g. "link"
    QString kindLabel;     // "Phone number"
    QString category;      // filter id, e.g. "contacts"
    QString categoryLabel; // "Contacts"
    QString title;         // one-line title; masked for possible passwords
    QString titleTail;     // muted continuation, used for link paths
    QString detail;        // kind-specific fact, e.g. "rgb(123, 203, 189)"
    QString actionLabel;   // the suggested action, empty when none fits
    // what the action does: "open" opens actionUrl, "copy" copies actionText,
    // "map" shows the place in actionText on a map, "remove" removes the entry
    QString actionKind;
    QString actionUrl;
    QString actionText;
    QColor swatch;         // valid only for colors
    bool sensitive = false;
    bool monospace = false;
    QDateTime capturedAt;
    // for dates and times: the moment to schedule, and how to say it
    QDateTime when;
    bool allDay = false;
    QString whenLabel;      // "Tomorrow at 2:30 PM"
};

// categories in the order the filter shows them
const QVector<EntryCategory>& entryCategories();

EntryPresentation presentEntry(const core::ClipboardEntry& entry);

// a session-relative bucket such as "Last 5 minutes" for time grouping
QString timeGroup(const QDateTime& capturedAt, const QDateTime& now);

} // namespace kstack::desktop
