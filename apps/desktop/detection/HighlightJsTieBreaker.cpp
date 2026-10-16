/*
 * Author: Runor Ewhro
 * Description: runs highlight.js's automatic detection restricted to the two
 *              candidate languages and accepts a clear winner.
 */
#include "HighlightJsTieBreaker.hpp"

namespace kstack::desktop {

const codelang::LanguageInfo* HighlightJsTieBreaker::choose(std::string_view text,
                                                              const codelang::LanguageInfo& first,
                                                              const codelang::LanguageInfo& second) {
    const QString firstName = HighlightJs::nameFor(first.name);
    const QString secondName = HighlightJs::nameFor(second.name);
    if (firstName.isEmpty() || secondName.isEmpty() || firstName == secondName) {
        return nullptr;
    }
    const auto detection = highlightJs_.detect(
        QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())), firstName, secondName);
    if (!detection || detection->lead < minimumLead) {
        return nullptr;
    }
    if (detection->language == firstName) {
        return &first;
    }
    return detection->language == secondName ? &second : nullptr;
}

} // namespace kstack::desktop
