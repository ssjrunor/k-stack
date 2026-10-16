/*
 * Author: Runor Ewhro
 * Description: renders copied code for the preview: highlight.js colours it
 *              in the palette QML passes in, tabs become four columns, and a
 *              line-number gutter is laid out to match line for line.
 */
#pragma once

#include "HighlightJs.hpp"

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace kstack::desktop {

class CodeHighlighter final : public QObject {
    Q_OBJECT

public:
    // a long paste is previewed from its start
    static constexpr int maximumLines = 500;

    explicit CodeHighlighter(HighlightJs& highlightJs, QObject* parent = nullptr);

    // language is the entry's label, a name such as "Python", or anything
    // else for code in no known language, which is shown without colour.
    // palette holds colours named structure, value, comment, added, and
    // removed, and the monospace family as font. The result holds html and
    // gutter, rich text with matching lines, plus lines (the snippet's
    // count) and shownLines.
    Q_INVOKABLE QVariantMap render(const QString& text, const QString& language, const QVariantMap& palette);

private:
    HighlightJs& highlightJs_;
    // the preview asks again for the same entry whenever a binding
    // re-evaluates, so the last answer is kept
    QString lastKey_;
    QVariantMap lastResult_;
};

} // namespace kstack::desktop
