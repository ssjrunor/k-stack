/*
 * Author: Runor Ewhro
 * Description: settles close calls when naming the language of copied code
 *              by asking highlight.js to choose between the detector's two
 *              best candidates. Its choice counts only with a clear relevance
 *              lead; otherwise the code stays unnamed.
 */
#pragma once

#include "HighlightJs.hpp"

#include <codelang/CodeLanguage.hpp>

#include <string_view>

namespace kstack::desktop {

class HighlightJsTieBreaker {
public:
    // the relevance lead highlight.js needs; measured on GitHub Linguist's
    // samples, this adds about five right names for every two wrong ones
    static constexpr int minimumLead = 4;

    explicit HighlightJsTieBreaker(HighlightJs& highlightJs) : highlightJs_(highlightJs) {}

    // one of the two languages, or nullptr when highlight.js does not know
    // both, cannot tell them apart, or has no clear preference
    [[nodiscard]] const codelang::LanguageInfo* choose(std::string_view text,
                                                         const codelang::LanguageInfo& first,
                                                         const codelang::LanguageInfo& second);

private:
    HighlightJs& highlightJs_;
};

} // namespace kstack::desktop
