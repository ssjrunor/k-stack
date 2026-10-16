/*
 * Author: Runor Ewhro
 * Description: portable system services built on Qt alone: events go to the
 *              default calendar app as an iCalendar file, so that app asks
 *              where to add them, and places open on OpenStreetMap; sharing,
 *              showing a calendar day, and reminders are left to platforms.
 */
#include "SystemServices.hpp"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

namespace kstack::desktop {

namespace {
// long enough for the calendar app to have read the file
constexpr int kEventFileLifetimeMs = 60000;

// RFC 5545 text escaping
QString icsText(QString text) {
    text.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    text.replace(QLatin1Char(';'), QStringLiteral("\\;"));
    text.replace(QLatin1Char(','), QStringLiteral("\\,"));
    text.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return text;
}
} // namespace

SystemServices::SystemServices(QObject* parent) : QObject(parent) {}

void SystemServices::setWindow(QWindow* window) { window_ = window; }

QWindow* SystemServices::window() const noexcept { return window_; }

bool SystemServices::busy() const noexcept { return busy_; }

void SystemServices::setBusy(bool busy) {
    if (busy_ != busy) {
        busy_ = busy;
        emit busyChanged();
    }
}

void SystemServices::resetBusy() { setBusy(false); }

bool SystemServices::canShare() const { return false; }

bool SystemServices::canAddEvent() const { return true; }

bool SystemServices::canShowCalendarDay() const { return false; }

bool SystemServices::canAddReminder() const { return false; }

void SystemServices::share(const QString& text, const QString& url, qreal x, qreal y) {
    Q_UNUSED(text);
    Q_UNUSED(url);
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void SystemServices::showCalendarDay(const QString& isoDateTime) { Q_UNUSED(isoDateTime); }

void SystemServices::addReminder(const QString& title, const QString& isoDateTime, bool allDay) {
    Q_UNUSED(title);
    Q_UNUSED(isoDateTime);
    Q_UNUSED(allDay);
}

void SystemServices::showOnMap(const QString& place) {
    QUrl url(QStringLiteral("https://www.openstreetmap.org/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), place);
    url.setQuery(query);
    QDesktopServices::openUrl(url);
}

void SystemServices::addCalendarEvent(const QString& title, const QString& isoDateTime, bool allDay) {
    const QString path = writeEventFile(title, isoDateTime, allDay);
    if (path.isEmpty()) {
        emit finished(QStringLiteral("The event could not be prepared."), false);
        return;
    }
    QTimer::singleShot(kEventFileLifetimeMs, [path] { QFile::remove(path); });
    openEventFile(path);
    emit finished(QStringLiteral("Calendar opened. Choose a calendar to add the event."), true);
}

void SystemServices::openEventFile(const QString& path) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

QString SystemServices::writeEventFile(const QString& title, const QString& isoDateTime, bool allDay) {
    const QDateTime when = QDateTime::fromString(isoDateTime, Qt::ISODate);
    if (!when.isValid()) {
        return {};
    }
    const QString summary = title.trimmed().isEmpty() ? QStringLiteral("New event") : title.trimmed();
    const QString dateFormat = QStringLiteral("yyyyMMdd");
    const QString timeFormat = QStringLiteral("yyyyMMdd'T'HHmmss");
    // a timed event is written in floating local time, the time as copied
    const QString start = allDay ? QStringLiteral("DTSTART;VALUE=DATE:") + when.date().toString(dateFormat)
                                 : QStringLiteral("DTSTART:") + when.toString(timeFormat);
    const QString end = allDay ? QStringLiteral("DTEND;VALUE=DATE:") + when.date().addDays(1).toString(dateFormat)
                               : QStringLiteral("DTEND:") + when.addSecs(3600).toString(timeFormat);
    const QString uid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QStringList lines = {
        QStringLiteral("BEGIN:VCALENDAR"),
        QStringLiteral("VERSION:2.0"),
        QStringLiteral("PRODID:-//Clipboard History//EN"),
        QStringLiteral("BEGIN:VEVENT"),
        QStringLiteral("UID:") + uid,
        QStringLiteral("DTSTAMP:") + QDateTime::currentDateTimeUtc().toString(timeFormat) + QLatin1Char('Z'),
        start,
        end,
        QStringLiteral("SUMMARY:") + icsText(summary),
        QStringLiteral("END:VEVENT"),
        QStringLiteral("END:VCALENDAR"),
    };
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(QStringLiteral("event-%1.ics").arg(uid));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    file.write(lines.join(QStringLiteral("\r\n")).toUtf8() + "\r\n");
    return path;
}

} // namespace kstack::desktop
