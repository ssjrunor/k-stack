/*
 * Author: Runor Ewhro
 * Description: one highlight.js, loaded on first use into a private
 *              JavaScript engine and shared by everything that needs it.
 *              Every call returns nothing rather than failing when the
 *              scripts cannot be loaded.
 */
#pragma once

#include <QJSValue>
#include <QString>

#include <memory>
#include <optional>
#include <string_view>

class QJSEngine;

namespace kstack::desktop {

class HighlightJs {
public:
    struct Detection {
        QString language; // highlight.js's name, such as "python"
        int lead = 0;     // its relevance over the other candidate
    };

    HighlightJs();
    ~HighlightJs();
    HighlightJs(const HighlightJs&) = delete;
    HighlightJs& operator=(const HighlightJs&) = delete;

    // highlight.js's name for one of Linguist's languages with clues, or
    // empty; HTML and XML share "xml", and INI and TOML share "ini"
    [[nodiscard]] static QString nameFor(std::string_view linguistName);

    // which of two languages the text reads as, and by how much
    [[nodiscard]] std::optional<Detection> detect(const QString& text, const QString& first,
                                                  const QString& second);
    // the text as HTML with <span class="hljs-..."> around each token;
    // empty when the language is unknown or highlighting fails
    [[nodiscard]] QString highlight(const QString& text, const QString& language);

private:
    bool ready();

    std::unique_ptr<QJSEngine> engine_;
    QJSValue highlightAuto_;
    QJSValue highlight_;
    bool failed_ = false;
};

} // namespace kstack::desktop
