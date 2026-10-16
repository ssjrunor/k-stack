/*
 * Author: Runor Ewhro
 * Description: names the kinds of text the history recognises, such as
 *              links, colors, codes, and possible passwords, and classifies
 *              captured text into one of them without any UI dependency.
 */
#pragma once

#include <string_view>

namespace kstack::core {

enum class ContentKind {
    Text,
    Link,
    Email,
    Phone,
    Address,
    Color,
    Code,
    Path,
    Number,
    Date,
    Time,
    VerificationCode,
    Secret,
};

// classifies by the whole trimmed text; anything that is not clearly one of
// the specific kinds stays Text, so a wrong guess never hides content
[[nodiscard]] ContentKind classifyContent(std::string_view text);

} // namespace kstack::core
