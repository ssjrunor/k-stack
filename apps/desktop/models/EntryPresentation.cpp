/*
 * Author: Runor Ewhro
 * Description: formats each recognised kind for display, parsing colors,
 *              dates, times, and numbers to offer a useful second reading
 *              and a fitting action such as opening a link or copying RGB.
 */
#include "EntryPresentation.hpp"

#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace kstack::desktop {

namespace {
using core::ContentKind;

QString words(const QString& flat) {
    const auto count = flat.split(QLatin1Char(' '), Qt::SkipEmptyParts).size();
    return count == 1 ? QStringLiteral("1 word") : QStringLiteral("%1 words").arg(count);
}

int lineCount(const QString& text) {
    return static_cast<int>(text.trimmed().count(QLatin1Char('\n'))) + 1;
}

// CSS colors put alpha last in #RRGGBBAA, where Qt expects it first, so hex
// and the functional forms are read here instead of through QColor's parser
QColor parseColor(const QString& flat) {
    QString hex = flat;
    if (hex.startsWith(QLatin1Char('#'))) {
        hex.remove(0, 1);
        if (hex.size() == 3 || hex.size() == 4) {
            QString expanded;
            for (const QChar c : hex) {
                expanded += c;
                expanded += c;
            }
            hex = expanded;
        }
        bool ok = false;
        const uint value = hex.toUInt(&ok, 16);
        if (!ok) {
            return {};
        }
        if (hex.size() == 6) {
            return QColor((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
        }
        if (hex.size() == 8) {
            return QColor((value >> 24) & 0xff, (value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
        }
        return {};
    }
    static const QRegularExpression numbers(QStringLiteral(R"(([\d.]+)(%?))"));
    QList<double> values;
    QList<bool> percents;
    for (auto it = numbers.globalMatch(flat); it.hasNext();) {
        const auto match = it.next();
        values << match.captured(1).toDouble();
        percents << !match.captured(2).isEmpty();
    }
    if (values.size() < 3) {
        return {};
    }
    const double alpha = values.size() > 3 ? (percents[3] ? values[3] / 100.0 : values[3]) : 1.0;
    if (flat.startsWith(QLatin1String("hsl"), Qt::CaseInsensitive)) {
        return QColor::fromHslF(static_cast<float>(std::fmod(values[0], 360.0) / 360.0),
                                static_cast<float>(values[1] / 100.0),
                                static_cast<float>(values[2] / 100.0), static_cast<float>(alpha));
    }
    const auto channel = [&](int i) {
        return std::clamp(percents[i] ? values[i] * 2.55 : values[i], 0.0, 255.0);
    };
    QColor color = QColor::fromRgbF(static_cast<float>(channel(0) / 255.0),
                                    static_cast<float>(channel(1) / 255.0),
                                    static_cast<float>(channel(2) / 255.0));
    color.setAlphaF(static_cast<float>(alpha));
    return color;
}

QString rgbText(const QColor& color) {
    if (color.alpha() < 255) {
        return QStringLiteral("rgba(%1, %2, %3, %4)")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(QString::number(color.alphaF(), 'g', 2));
    }
    return QStringLiteral("rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
}

QString hexText(const QColor& color) {
    return color.name(QColor::HexRgb).toUpper();
}

QString relativeDay(const QDate& date) {
    const qint64 days = QDate::currentDate().daysTo(date);
    if (days == 0) {
        return QStringLiteral("today");
    }
    if (days == 1) {
        return QStringLiteral("tomorrow");
    }
    if (days == -1) {
        return QStringLiteral("yesterday");
    }
    return days > 0 ? QStringLiteral("in %1 days").arg(days)
                    : QStringLiteral("%1 days ago").arg(-days);
}

QDate parseDate(const QString& flat) {
    const QLocale english = QLocale::c();
    QDate date = QDate::fromString(flat.left(10), Qt::ISODate);
    if (date.isValid()) {
        return date;
    }
    static const QRegularExpression numeric(QStringLiteral(R"(^(\d{1,2})[/.-](\d{1,2})[/.-](\d{2}|\d{4})$)"));
    if (const auto match = numeric.match(flat); match.hasMatch()) {
        int first = match.captured(1).toInt();
        int second = match.captured(2).toInt();
        int year = match.captured(3).toInt();
        if (year < 100) {
            year += 2000;
        }
        // follow the user's locale for which number is the month, unless only
        // one reading is possible
        const bool monthFirst = QLocale().dateFormat(QLocale::ShortFormat).startsWith(QLatin1Char('M'));
        const bool swap = first > 12 ? false : second > 12 ? true : monthFirst;
        if (swap) {
            std::swap(first, second);
        }
        return QDate(year, second, first);
    }
    QString text = flat;
    static const QRegularExpression weekday(
        QStringLiteral(R"(^(mon|tue|wed|thu|fri|sat|sun)[a-z]*\.?,?\s+)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression ordinal(QStringLiteral(R"((\d)(st|nd|rd|th)\b)"),
                                            QRegularExpression::CaseInsensitiveOption);
    text.remove(weekday);
    text.replace(ordinal, QStringLiteral("\\1"));
    text.remove(QLatin1Char(',')).remove(QLatin1Char('.'));
    text.replace(QRegularExpression(QStringLiteral(R"(\bsept\b)"), QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("Sep"));
    text = text.simplified();
    // Qt's month names are capitalised, so normalise the case first
    text = text.toLower();
    for (const QString& name : {QStringLiteral("jan"), QStringLiteral("feb"), QStringLiteral("mar"),
                                QStringLiteral("apr"), QStringLiteral("may"), QStringLiteral("jun"),
                                QStringLiteral("jul"), QStringLiteral("aug"), QStringLiteral("sep"),
                                QStringLiteral("oct"), QStringLiteral("nov"), QStringLiteral("dec")}) {
        const auto at = text.indexOf(name);
        if (at >= 0) {
            text[at] = text[at].toUpper();
        }
    }
    const bool hasYear = text.contains(QRegularExpression(QStringLiteral(R"(\d{4})")));
    if (!hasYear) {
        text += QStringLiteral(" %1").arg(QDate::currentDate().year());
    }
    for (const char* format : {"d MMMM yyyy", "d MMM yyyy", "MMMM d yyyy", "MMM d yyyy"}) {
        date = english.toDate(text, QString::fromLatin1(format));
        if (date.isValid()) {
            return date;
        }
    }
    return {};
}

QTime parseTime(const QString& flat, bool* twelveHour) {
    QString text = flat.toLower();
    text.remove(QLatin1Char('.')).remove(QLatin1Char(' '));
    *twelveHour = text.endsWith(QLatin1String("am")) || text.endsWith(QLatin1String("pm"));
    const QLocale english = QLocale::c();
    for (const char* format : {"H:mm", "H:mm:ss", "h:mmap", "h:mm:ssap", "hap"}) {
        const QTime time = english.toTime(text, QString::fromLatin1(format));
        if (time.isValid()) {
            return time;
        }
    }
    return {};
}

QString expandedPath(const QString& flat) {
    if (flat.startsWith(QLatin1String("file://"))) {
        return QUrl(flat).toLocalFile();
    }
    if (flat.startsWith(QLatin1Char('~'))) {
        return QDir::homePath() + flat.mid(1);
    }
    return flat;
}

void presentKind(const QString& text, const QString& flat, ContentKind kind, EntryPresentation& p) {
    switch (kind) {
    case ContentKind::Link: {
        const QString href = flat.startsWith(QLatin1String("www."), Qt::CaseInsensitive)
                                 ? QStringLiteral("https://") + flat
                                 : flat;
        const QUrl url(href);
        QString host = url.host();
        if (host.startsWith(QLatin1String("www."))) {
            host.remove(0, 4);
        }
        QString tail = url.path();
        if (url.hasQuery()) {
            tail += QLatin1Char('?') + url.query();
        }
        if (tail == QLatin1String("/")) {
            tail.clear();
        }
        p.kind = QStringLiteral("link");
        p.kindLabel = QStringLiteral("Link");
        p.title = host.isEmpty() ? flat : host;
        p.titleTail = host.isEmpty() ? QString() : tail;
        p.detail = host;
        p.actionLabel = QStringLiteral("Open link");
        p.actionKind = QStringLiteral("open");
        p.actionUrl = href;
        return;
    }
    case ContentKind::Email: {
        QString address = flat;
        address.remove(QRegularExpression(QStringLiteral("^mailto:"), QRegularExpression::CaseInsensitiveOption));
        p.kind = QStringLiteral("email");
        p.kindLabel = QStringLiteral("Email address");
        p.title = address;
        p.detail = address.section(QLatin1Char('@'), 1);
        p.actionLabel = QStringLiteral("New email");
        p.actionKind = QStringLiteral("open");
        p.actionUrl = QStringLiteral("mailto:") + address;
        return;
    }
    case ContentKind::Phone: {
        QString dial = flat;
        dial.remove(QRegularExpression(QStringLiteral(R"([^\d+])")));
        p.kind = QStringLiteral("phone");
        p.kindLabel = QStringLiteral("Phone number");
        p.title = flat;
        p.detail = QStringLiteral("%1 digits").arg(dial.count(QRegularExpression(QStringLiteral(R"(\d)"))));
        p.actionLabel = QStringLiteral("Call");
        p.actionKind = QStringLiteral("open");
        p.actionUrl = QStringLiteral("tel:") + dial;
        return;
    }
    case ContentKind::Address: {
        QStringList lines;
        for (const QString& line : text.split(QLatin1Char('\n'))) {
            if (!line.trimmed().isEmpty()) {
                lines << line.trimmed();
            }
        }
        const QString oneLine = lines.join(QStringLiteral(", "));
        p.kind = QStringLiteral("address");
        p.kindLabel = QStringLiteral("Address");
        p.title = oneLine;
        p.detail = lines.size() > 1 ? lines.last() : QStringLiteral("Address");
        p.actionLabel = QStringLiteral("Show on map");
        p.actionKind = QStringLiteral("map");
        p.actionText = oneLine;
        return;
    }
    case ContentKind::Color: {
        p.kind = QStringLiteral("color");
        p.kindLabel = QStringLiteral("Color");
        p.title = flat;
        p.monospace = true;
        p.swatch = parseColor(flat);
        if (p.swatch.isValid()) {
            const bool fromHex = flat.startsWith(QLatin1Char('#'));
            p.detail = fromHex ? rgbText(p.swatch) : hexText(p.swatch);
            p.actionLabel = fromHex ? QStringLiteral("Copy as RGB") : QStringLiteral("Copy as hex");
            p.actionKind = QStringLiteral("copy");
            p.actionText = p.detail;
        }
        return;
    }
    case ContentKind::Code: {
        const int lines = lineCount(text);
        p.kind = QStringLiteral("code");
        p.kindLabel = QStringLiteral("Code");
        p.title = text.trimmed().section(QLatin1Char('\n'), 0, 0).simplified();
        p.detail = lines > 1 ? QStringLiteral("%1 lines").arg(lines) : QStringLiteral("Command");
        p.monospace = true;
        return;
    }
    case ContentKind::Path: {
        // a file reference may come wrapped in brackets or quotes and end
        // with a line and column, as in "(src/main.cpp:31"
        static const QRegularExpression reference(
            QStringLiteral(R"re(^[("'\[`]?(.+?)(?::(\d+)(?::(\d+))?)?[)"'\]`,.:]*$)re"));
        const auto parts = reference.match(flat);
        const QString cleaned = parts.hasMatch() ? parts.captured(1) : flat;
        const QString line = parts.captured(2);
        const QString path = expandedPath(cleaned);
        const QFileInfo info(path);
        p.kind = QStringLiteral("path");
        p.kindLabel = QStringLiteral("File path");
        p.title = flat;
        const QString name = info.fileName().isEmpty() ? path : info.fileName();
        p.detail = line.isEmpty() ? name : QStringLiteral("%1, line %2").arg(name, line);
        p.monospace = true;
        // a relative path names no file without knowing where it came from
        if (!QDir::isRelativePath(path)) {
            p.actionLabel = QStringLiteral("Open");
            p.actionKind = QStringLiteral("open");
            p.actionUrl = QUrl::fromLocalFile(path).toString();
        }
        return;
    }
    case ContentKind::Number: {
        QString plain = flat;
        plain.remove(QLatin1Char(','));
        p.kind = QStringLiteral("number");
        p.kindLabel = QStringLiteral("Number");
        p.title = flat;
        bool ok = false;
        const double value = plain.toDouble(&ok);
        if (ok && std::abs(value) >= 10000 && !flat.contains(QLatin1Char(','))) {
            p.detail = QLocale().toString(value, 'f', plain.contains(QLatin1Char('.'))
                                                          ? static_cast<int>(plain.section(QLatin1Char('.'), 1).size())
                                                          : 0);
        } else {
            p.detail = QStringLiteral("Number");
        }
        if (flat.contains(QLatin1Char(','))) {
            p.actionLabel = QStringLiteral("Copy without commas");
            p.actionKind = QStringLiteral("copy");
            p.actionText = plain;
        }
        return;
    }
    case ContentKind::Date: {
        p.kind = QStringLiteral("date");
        p.kindLabel = QStringLiteral("Date");
        p.title = flat;
        const QDate date = parseDate(flat);
        if (date.isValid()) {
            p.detail = QLocale().toString(date, QStringLiteral("dddd, d MMMM yyyy")) +
                       QStringLiteral(", ") + relativeDay(date);
            p.when = QDateTime(date, QTime(9, 0));
            p.allDay = true;
            p.whenLabel = QLocale().toString(date, QStringLiteral("dddd, d MMMM yyyy")) +
                          QStringLiteral(", all day");
            const QString iso = date.toString(Qt::ISODate);
            if (flat != iso) {
                p.actionLabel = QStringLiteral("Copy as ISO date");
                p.actionKind = QStringLiteral("copy");
                p.actionText = iso;
            }
        } else {
            p.detail = QStringLiteral("Date");
        }
        return;
    }
    case ContentKind::Time: {
        p.kind = QStringLiteral("time");
        p.kindLabel = QStringLiteral("Time");
        p.title = flat;
        bool twelveHour = false;
        const QTime time = parseTime(flat, &twelveHour);
        if (time.isValid()) {
            p.detail = twelveHour ? time.toString(QStringLiteral("HH:mm"))
                                  : QLocale::c().toString(time, QStringLiteral("h:mm AP"));
            p.actionLabel = twelveHour ? QStringLiteral("Copy as 24-hour time")
                                       : QStringLiteral("Copy as 12-hour time");
            p.actionKind = QStringLiteral("copy");
            p.actionText = p.detail;
            // a bare time means its next occurrence, and the label says which
            // day that is so nothing is scheduled by surprise
            const QDateTime now = QDateTime::currentDateTime();
            QDateTime at(now.date(), time);
            const bool tomorrow = at < now.addSecs(-60);
            if (tomorrow) {
                at = at.addDays(1);
            }
            p.when = at;
            p.whenLabel = (tomorrow ? QStringLiteral("Tomorrow at ") : QStringLiteral("Today at ")) +
                          QLocale().toString(time, QLocale::ShortFormat);
        } else {
            p.detail = QStringLiteral("Time");
        }
        return;
    }
    case ContentKind::VerificationCode:
        p.kind = QStringLiteral("verification");
        p.kindLabel = QStringLiteral("Verification code");
        p.title = flat;
        p.detail = QStringLiteral("Verification code");
        p.monospace = true;
        return;
    case ContentKind::Secret:
        p.kind = QStringLiteral("secret");
        p.kindLabel = QStringLiteral("Possible password");
        p.title = QString(std::clamp(static_cast<int>(flat.size()), 8, 16), QChar(0x2022));
        p.detail = QStringLiteral("Hidden");
        p.sensitive = true;
        p.actionLabel = QStringLiteral("Remove from history");
        p.actionKind = QStringLiteral("remove");
        return;
    case ContentKind::Text:
        break;
    }
    const int lines = lineCount(text);
    p.kind = QStringLiteral("text");
    p.kindLabel = QStringLiteral("Text");
    p.title = flat;
    p.detail = lines > 1 ? QStringLiteral("%1 lines, %2").arg(lines).arg(words(flat)) : words(flat);
}

QString categoryOf(ContentKind kind) {
    switch (kind) {
    case ContentKind::Link:
        return QStringLiteral("links");
    case ContentKind::Code:
        return QStringLiteral("code");
    case ContentKind::Path:
        return QStringLiteral("files");
    case ContentKind::Color:
        return QStringLiteral("colors");
    case ContentKind::Email:
    case ContentKind::Phone:
    case ContentKind::Address:
        return QStringLiteral("contacts");
    case ContentKind::Number:
    case ContentKind::VerificationCode:
        return QStringLiteral("numbers");
    case ContentKind::Date:
    case ContentKind::Time:
        return QStringLiteral("dates");
    case ContentKind::Secret:
        return QStringLiteral("secrets");
    case ContentKind::Text:
        break;
    }
    return QStringLiteral("text");
}
} // namespace

const QVector<EntryCategory>& entryCategories() {
    static const QVector<EntryCategory> categories = {
        {QStringLiteral("links"), QStringLiteral("Links")},
        {QStringLiteral("text"), QStringLiteral("Text")},
        {QStringLiteral("code"), QStringLiteral("Code")},
        {QStringLiteral("files"), QStringLiteral("Files")},
        {QStringLiteral("colors"), QStringLiteral("Colors")},
        {QStringLiteral("contacts"), QStringLiteral("Contacts")},
        {QStringLiteral("numbers"), QStringLiteral("Numbers")},
        {QStringLiteral("dates"), QStringLiteral("Dates")},
        {QStringLiteral("secrets"), QStringLiteral("Passwords")},
    };
    return categories;
}

EntryPresentation presentEntry(const core::ClipboardEntry& entry) {
    EntryPresentation p;
    const QString text = QString::fromStdString(entry.text);
    presentKind(text, text.simplified(), entry.kind, p);
    // code names its language when it was recognised at capture
    if (!entry.codeLanguage.empty()) {
        p.kindLabel = QString::fromStdString(entry.codeLanguage);
    }
    p.category = categoryOf(entry.kind);
    for (const auto& category : entryCategories()) {
        if (category.id == p.category) {
            p.categoryLabel = category.label;
        }
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        entry.capturedAt.time_since_epoch())
                        .count();
    p.capturedAt = QDateTime::fromMSecsSinceEpoch(ms);
    return p;
}

QString timeGroup(const QDateTime& capturedAt, const QDateTime& now) {
    const qint64 seconds = capturedAt.secsTo(now);
    if (seconds < 5 * 60) {
        return QStringLiteral("Last 5 minutes");
    }
    if (seconds < 60 * 60) {
        return QStringLiteral("Last hour");
    }
    if (capturedAt.date() == now.date()) {
        return QStringLiteral("Earlier today");
    }
    if (capturedAt.date() == now.date().addDays(-1)) {
        return QStringLiteral("Yesterday");
    }
    return QStringLiteral("Earlier");
}

} // namespace kstack::desktop
