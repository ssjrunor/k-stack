/*
 * Author: Runor Ewhro
 * Description: turns highlight.js's class-marked HTML into inline styles, so
 *              Qt's rich text shows it without a stylesheet. Colour has two
 *              jobs: structure (keywords, tags) and values (strings,
 *              numbers); comments fade, and names are set in weight.
 */
#include "CodeHighlighter.hpp"

#include <QColor>
#include <QHash>
#include <QRegularExpression>
#include <QStringList>

namespace kstack::desktop {
namespace {

enum class Role { Structure, Value, Comment, Name, Added, Removed, Plain };

// highlight.js's scope, the first class after "hljs-", by the job its
// colour does
Role roleOf(const QString& scope) {
    static const QHash<QString, Role> roles{
        {"keyword", Role::Structure},   {"meta", Role::Structure},        {"tag", Role::Structure},
        {"name", Role::Structure},      {"selector-tag", Role::Structure}, {"selector-id", Role::Structure},
        {"selector-class", Role::Structure}, {"selector-pseudo", Role::Structure},
        {"section", Role::Structure},   {"template-tag", Role::Structure},
        {"string", Role::Value},        {"number", Role::Value},          {"literal", Role::Value},
        {"regexp", Role::Value},        {"symbol", Role::Value},          {"char", Role::Value},
        {"link", Role::Value},          {"code", Role::Value},            {"bullet", Role::Value},
        {"comment", Role::Comment},     {"quote", Role::Comment},
        {"title", Role::Name},          {"function", Role::Name},         {"type", Role::Name},
        {"built_in", Role::Name},       {"strong", Role::Name},
        {"addition", Role::Added},      {"deletion", Role::Removed},
    };
    return roles.value(scope, Role::Plain);
}

QString colorOf(const QVariantMap& palette, const char* name) {
    return palette.value(QLatin1String(name)).value<QColor>().name(QColor::HexRgb);
}

// a span's inline style for its role
QString styleOf(Role role, const QVariantMap& palette) {
    switch (role) {
    case Role::Structure:
        return QStringLiteral("color:%1").arg(colorOf(palette, "structure"));
    case Role::Value:
        return QStringLiteral("color:%1").arg(colorOf(palette, "value"));
    case Role::Comment:
        return QStringLiteral("color:%1;font-style:italic").arg(colorOf(palette, "comment"));
    case Role::Name:
        return QStringLiteral("font-weight:600");
    case Role::Added:
        return QStringLiteral("color:%1").arg(colorOf(palette, "added"));
    case Role::Removed:
        return QStringLiteral("color:%1").arg(colorOf(palette, "removed"));
    case Role::Plain:
        break;
    }
    return {};
}

// tabs to the next multiple of four columns, so indentation reads as the
// author's editor most likely showed it
QString expandTabs(const QString& line) {
    if (!line.contains(QLatin1Char('\t'))) {
        return line;
    }
    QString out;
    for (const QChar c : line) {
        if (c == QLatin1Char('\t')) {
            out += QString(4 - out.size() % 4, QLatin1Char(' '));
        } else {
            out += c;
        }
    }
    return out;
}

QString preformatted(const QString& body, const QVariantMap& palette) {
    // the same block style for code and gutter keeps their lines level
    return QStringLiteral("<pre style=\"margin:0;line-height:145%;font-family:'%1'\">%2</pre>")
        .arg(palette.value(QStringLiteral("font")).toString().toHtmlEscaped(), body);
}

} // namespace

CodeHighlighter::CodeHighlighter(HighlightJs& highlightJs, QObject* parent)
    : QObject(parent), highlightJs_(highlightJs) {}

QVariantMap CodeHighlighter::render(const QString& text, const QString& language, const QVariantMap& palette) {
    QString key = language + QLatin1Char('\n') + text;
    for (auto it = palette.cbegin(); it != palette.cend(); ++it) {
        key += QLatin1Char('\n') + it.key() + QLatin1Char('=') + it.value().toString();
    }
    if (key == lastKey_) {
        return lastResult_;
    }

    QStringList lines = text.split(QLatin1Char('\n'));
    while (lines.size() > 1 && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }
    const int total = static_cast<int>(lines.size());
    if (lines.size() > maximumLines) {
        lines = lines.mid(0, maximumLines);
    }
    for (QString& line : lines) {
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
        line = expandTabs(line);
    }
    const QString source = lines.join(QLatin1Char('\n'));

    QString body = highlightJs_.highlight(source, HighlightJs::nameFor(language.toStdString()));
    if (body.isEmpty()) {
        body = source.toHtmlEscaped();
    } else {
        static const QRegularExpression span(QStringLiteral("<span class=\"hljs-([\\w-]+)[^\"]*\">"));
        QString styled;
        qsizetype from = 0;
        for (auto match = span.globalMatch(body); match.hasNext();) {
            const auto found = match.next();
            styled += QStringView(body).mid(from, found.capturedStart() - from);
            const QString style = styleOf(roleOf(found.captured(1)), palette);
            styled += style.isEmpty() ? QStringLiteral("<span>") : QStringLiteral("<span style=\"%1\">").arg(style);
            from = found.capturedEnd();
        }
        body = styled + QStringView(body).mid(from);
    }

    // numbers padded to one width, so they line up on the right
    const int width = static_cast<int>(QString::number(lines.size()).size());
    QStringList numbers;
    for (int i = 1; i <= lines.size(); ++i) {
        numbers << QStringLiteral("%1").arg(i, width);
    }

    lastKey_ = key;
    lastResult_ = QVariantMap{
        {QStringLiteral("html"), preformatted(body, palette)},
        {QStringLiteral("gutter"), preformatted(numbers.join(QLatin1Char('\n')), palette)},
        {QStringLiteral("lines"), total},
        {QStringLiteral("shownLines"), static_cast<int>(lines.size())},
    };
    return lastResult_;
}

} // namespace kstack::desktop
