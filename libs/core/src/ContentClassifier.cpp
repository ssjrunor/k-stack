/*
 * Author: Runor Ewhro
 * Description: recognises the kind of a captured text item from its shape,
 *              checking the most specific and most sensitive kinds first and
 *              keeping the expensive patterns away from very long text. Code
 *              is told from prose line by line, with the code-language
 *              detector's verdict as further evidence.
 */
#include <kstack/core/ContentKind.hpp>
#include <codelang/CodeLanguage.hpp>

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>

namespace kstack::core {

namespace {
constexpr std::size_t kPatternLimit = 2000;
constexpr auto kIcase = std::regex::ECMAScript | std::regex::icase;

std::string_view trimmed(std::string_view text) {
    const auto space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!text.empty() && space(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && space(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

bool matches(const std::string& text, const std::regex& pattern) {
    return std::regex_search(text, pattern);
}

std::size_t countIf(std::string_view text, int (*test)(int)) {
    return static_cast<std::size_t>(std::count_if(text.begin(), text.end(), [test](char c) {
        return test(static_cast<unsigned char>(c)) != 0;
    }));
}

// a single unbroken token that mixes letters, digits, and symbols the way
// generated passwords and API keys do, but ordinary identifiers do not
bool looksLikeSecret(std::string_view token) {
    if (token.size() < 8 || token.size() > 128) {
        return false;
    }
    // a reference to code, such as Foo::bar(1) or node->next, mixes the same
    // kinds of characters but is structured like code, not random
    static const std::regex codeReference(R"(^[\w.]+(::|->)\w|^[A-Za-z_][\w.:]*\(.*\)$)");
    if (std::regex_search(token.begin(), token.end(), codeReference)) {
        return false;
    }
    if (std::any_of(token.begin(), token.end(),
                    [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; })) {
        return false;
    }
    const bool lower = countIf(token, std::islower) > 0;
    const bool upper = countIf(token, std::isupper) > 0;
    const std::size_t digits = countIf(token, std::isdigit);
    const bool symbol = countIf(token, std::ispunct) > 0;
    const int classes = int{lower} + int{upper} + int{digits > 0} + int{symbol};
    if (classes == 4) {
        return true;
    }
    // long base64-like keys without punctuation still read as secrets
    return token.size() >= 24 && classes == 3 && !symbol && digits >= 3;
}

enum class LineReading { Code, Prose, Neither };

// a token of ordinary writing: a word or a number, with the punctuation
// sentences put around them
bool isWritingToken(std::string_view token) {
    // bytes from 0x80 are UTF-8, such as accented letters and curly quotes
    static const std::regex word(
        R"re(^[("'\[\x80-\xff]*([A-Za-z\x80-\xff][A-Za-z0-9\x80-\xff'-]*|[$\x80-\xff]?\d[\d.,:/%-]*([ap]m|s|st|nd|rd|th)?|&)[.,;:!?)"'\]\x80-\xff]*$)re",
        kIcase);
    return std::regex_match(token.begin(), token.end(), word);
}

// one line of text: code when it shows a sign of code and does not read as
// a sentence; prose when it reads as a sentence and has no sign that only
// code shows. A [section] header counts only alongside other lines.
LineReading readLine(std::string_view line, bool alone) {
    // writing quotes code in `backticks` and links as [text](address); both
    // read as words in a sentence
    static const std::regex inlineCode(R"(`[^`]+`)");
    static const std::regex markdownLink(R"(\[([^\]]+)\]\([^)\s]+\))");
    std::string text(trimmed(line));
    // a Go struct tag is code even though it sits in backticks
    static const std::regex structTag(R"(`\w+:")");
    if (std::regex_search(text, structTag)) {
        return LineReading::Code;
    }
    if (text.find('`') != std::string::npos || text.find("](") != std::string::npos) {
        text = std::regex_replace(std::regex_replace(text, inlineCode, "code"), markdownLink, "$1");
    }
    if (text.empty()) {
        return LineReading::Neither;
    }
    // signs only code shows: a statement or block ending, a call, or an
    // operator that writing does not use
    static const std::regex strong(R"([;{}]$|\w\(.*\)|==|!=|&&|\|\||::|->|:=|\+=|-=|<-|<<|>>)");
    // lines only code starts this way; an import list otherwise reads much
    // like a list of words
    static const std::regex codeStart(
        R"(^(from [\w.]+ import |import [\w.]+(,| as |;|$)|def \w+\(|class \w+[:(\s{]|#include|#import|#define|package [\w.]+;?$|using [\w.]+;|fn \w+|func \w+|pub fn ))");
    // JSON's "key": value pairs, and lines of nothing but brackets
    static const std::regex data(R"("[\w$%@./-]+"\s*:\s*("|-?\d|\{|\[|true|false|null)|^[{}\[\]();,]+$)");
    const bool strongSign = std::regex_search(text, strong) || std::regex_search(text, codeStart) ||
                            std::regex_search(text, data);
    std::size_t tokens = 0;
    std::size_t writing = 0;
    for (std::size_t i = 0; i < text.size();) {
        const auto end = text.find_first_of(" \t", i);
        const std::string_view token = std::string_view(text).substr(i, end == std::string::npos ? std::string::npos : end - i);
        if (!token.empty()) {
            ++tokens;
            writing += isWritingToken(token);
        }
        if (end == std::string::npos) {
            break;
        }
        i = end + 1;
    }
    if (!strongSign && tokens >= 4 && writing * 5 >= tokens * 4) {
        return LineReading::Prose;
    }
    // a long sentence that quotes a little code, such as "In Python, ==
    // compares two objects by value.", is still a sentence; code lines
    // rarely end with a full stop
    static const std::regex sentenceEnd(R"re([.?!]["')\x80-\xff]*$)re");
    if (tokens >= 10 && writing * 20 >= tokens * 17 && std::regex_search(text, sentenceEnd) &&
        !std::regex_search(text, codeStart)) {
        return LineReading::Prose;
    }
    static const std::regex keyword(
        R"(^(import |from [\w.]+ import |def |class \w|function\b|const |let |var |fn |func |pub |public |private |protected |static |return\b|package |using |namespace |module |require |#include|#import|#define|#if|#endif|#pragma|elif |else\s*[:{]|(if|for|while|switch|catch)\s*\(|\} ?else|end$|done$|fi$|esac$|@\w+))");
    static const std::regex sql(
        R"(^(select|insert into|update \w+ set|delete from|create (or replace )?(table|index|view|function)|alter table|drop table|where|from \w|group by|order by|having|values|(inner |left |right |full |cross )?join)\b)",
        kIcase);
    static const std::regex assignment(R"(^[\w$@.\[\]"'-]+\s*(=|:=|\+=|-=|\?=|\|\|=)\s*\S)");
    static const std::regex yamlKey(R"(^(- )?[a-z_][\w.-]*:(\s+\S.*)?$)");
    static const std::regex section(R"(^\[\[?[\w .:"-]+\]\]?$)");
    static const std::regex tag(R"(^</?[A-Za-z][\w:.-]*(\s[^<>]*)?/?>)");
    static const std::regex command(
        R"(^(\$ |(git|npm|npx|yarn|pnpm|brew|cmake|ctest|make|cd|ls|sudo|python3?|pip3?|node|curl|wget|docker|kubectl|ssh|scp|cargo|go|rm|cp|mv|mkdir|chmod|chown|export|echo|cat|grep|find|xcrun|xcodebuild|swift|open|defaults|launchctl|pbcopy|tar|unzip)( |$)))");
    // an array or object on one line, not a bracketed note like [Illustration]
    static const std::regex container(R"(^(\{.*[:"].*\}|\[.*[,"].*\]|\{\}|\[\]),?$)");
    // SQL: a row of values, or a column with an uppercase type
    static const std::regex sqlRow(R"(^\(.*,.*\),?;?$)");
    static const std::regex sqlColumn(
        R"(^"?\w+"?\s+(INT|INTEGER|BIGINT|SMALLINT|TINYINT|SERIAL|VARCHAR|CHAR|TEXT|DATE|DATETIME|TIMESTAMP|TIME|BOOLEAN|BOOL|DECIMAL|NUMERIC|REAL|FLOAT|DOUBLE|BLOB|UUID|JSONB?)\b)");
    static const std::regex operators(R"(=>|===|[^\s]=[^\s=]| = )");
    const bool shortLine = tokens <= 8;
    if (strongSign || std::regex_search(text, keyword) || std::regex_search(text, sql) ||
        std::regex_search(text, assignment) || (!alone && std::regex_search(text, section)) ||
        std::regex_search(text, tag) || std::regex_search(text, command) ||
        std::regex_search(text, container) || std::regex_search(text, sqlRow) ||
        std::regex_search(text, sqlColumn) || (shortLine && std::regex_search(text, yamlKey)) ||
        std::regex_search(text, operators)) {
        return LineReading::Code;
    }
    return LineReading::Neither;
}

// comments say nothing either way: both code and notes use them
bool isComment(std::string_view line) {
    const std::string_view text = trimmed(line);
    return text.starts_with("//") || text.starts_with("/*") || text.starts_with("* ") ||
           text.starts_with("# ") || text.starts_with("<!--") || text.starts_with("-- ");
}

bool looksLikeCode(const std::string& text) {
    std::size_t code = 0;
    std::size_t prose = 0;
    std::size_t considered = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto end = text.find('\n', start);
        const std::string_view line =
            std::string_view(text).substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!trimmed(line).empty() && !isComment(line)) {
            ++considered;
            const LineReading reading = readLine(line, text.find('\n') == std::string::npos);
            code += reading == LineReading::Code;
            prose += reading == LineReading::Prose;
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    const auto guess = codelang::detectCodeLanguage(text);
    const bool proseLanguage = guess && guess->language->type == codelang::LanguageType::Prose;
    // a shebang, JSON, PHP, XML, or HTML settles it
    if (guess && !proseLanguage && guess->basis == codelang::CodeLanguageGuess::Basis::Marker) {
        return true;
    }
    if (considered == 0 || prose * 2 > considered) {
        return false;
    }
    // without any sentence, a third of the lines showing code is enough, as
    // when an import is followed by the names it brings in
    if (code * 2 >= considered || (prose == 0 && code * 3 >= considered)) {
        return true;
    }
    // a clear language with little prose around it
    return guess && !proseLanguage && prose * 4 <= considered;
}
} // namespace

ContentKind classifyContent(std::string_view raw) {
    const std::string_view view = trimmed(raw);
    if (view.empty()) {
        return ContentKind::Text;
    }
    const std::string text(view);

    if (text.find("-----BEGIN") != std::string::npos &&
        text.find("PRIVATE KEY") != std::string::npos) {
        return ContentKind::Secret;
    }
    if (text.size() > kPatternLimit) {
        return looksLikeCode(text.substr(0, kPatternLimit)) ? ContentKind::Code : ContentKind::Text;
    }

    const bool singleLine = text.find('\n') == std::string::npos;
    if (singleLine) {
        static const std::regex tokenPrefix(
            R"(^(sk-|sk_live_|sk_test_|rk_live_|ghp_|gho_|ghs_|github_pat_|glpat-|xox[abpr]-|AKIA|AIza)\S{8,}$)");
        static const std::regex link(R"(^((https?|ftp)://|www\.)\S+\.\S+$)", kIcase);
        static const std::regex email(R"(^(mailto:)?[\w.+-]+@[\w-]+(\.[\w-]+)+$)", kIcase);
        static const std::regex color(
            R"(^(#([0-9a-f]{3,4}|[0-9a-f]{6}|[0-9a-f]{8})|(rgba?|hsla?)\(\s*[\d.]+%?(\s*[,\s/]\s*[\d.]+%?){2,3}\s*\))$)",
            kIcase);
        static const std::regex path(R"(^((~|\.{1,2})?/[^\s/]+(/[^\s/]*)*|file://\S+|[a-z]:\\\S*)$)", kIcase);
        // a file reference as compilers, stack traces, and editors print
        // it: a path with a file extension, or a code file's name, either
        // with a line and column, in the brackets or quotes it is often
        // copied with
        static const std::regex fileReference(
            R"re(^[("'\[`]?((?![\w-]+\.[\w.-]+/)((~|\.{1,2})?/)?([\w@.+-]+/)+[\w@.+-]+\.[a-z][\w]{0,9}(:\d+(:\d+)?)?|[\w@.+-]+\.(c|cc|cpp|cxx|h|hh|hpp|m|mm|swift|py|rb|js|jsx|ts|tsx|go|rs|java|kt|cs|php|lua|sh|qml|json|ya?ml|toml|css|scss|html|xml|sql)(:\d+(:\d+)?))[)"'\]`,.:]*$)re",
            kIcase);
        static const std::regex command(
            R"(^(\$ |(git|npm|npx|yarn|pnpm|brew|cmake|ctest|make|cd|ls|sudo|python3?|pip3?|node|curl|wget|docker|kubectl|ssh|scp|cargo|go|rm|cp|mv|mkdir|chmod|chown|export|echo|cat|grep|find|xcrun|xcodebuild|swift|open|defaults|launchctl|pbcopy|tar|unzip)\s))");
        static const std::regex verification(R"(^(\d{6}|\d{8}|\d{3}[ -]\d{3}|[a-z]{1,3}-\d{4,8})$)", kIcase);
        static const std::regex time(
            R"(^(([01]?\d|2[0-3]):[0-5]\d(:[0-5]\d)?(\s?[ap]\.?m\.?)?|(1[0-2]|0?[1-9])\s?[ap]\.?m\.?)$)", kIcase);
        static const std::string month =
            R"((jan(uary)?|feb(ruary)?|mar(ch)?|apr(il)?|may|june?|july?|aug(ust)?|sep(t(ember)?)?|oct(ober)?|nov(ember)?|dec(ember)?)\.?)";
        static const std::string weekday =
            R"(((mon|tue(s)?|wed(nes)?|thu(r(s)?)?|fri|sat(ur)?|sun)(day)?\.?,?\s+)?)";
        static const std::regex date(
            R"(^(\d{4}-\d{2}-\d{2}(t[\d:.]+z?)?|\d{1,2}[/.-]\d{1,2}[/.-](\d{2}|\d{4}))$|^)" + weekday +
                R"((\d{1,2}(st|nd|rd|th)?\s+)" + month + R"(,?(\s+\d{4})?|)" + month +
                R"(\s+\d{1,2}(st|nd|rd|th)?,?(\s+\d{4})?)$)",
            kIcase);
        static const std::regex phone(R"(^\+?[\d\s().-]{7,22}$)");
        static const std::regex number(
            R"(^[-+]?[$€£¥]?\s?(\d{1,3}(,\d{3})+|\d+)(\.\d+)?\s?(%|k|m|bn)?$)", kIcase);
        if (matches(text, tokenPrefix)) {
            return ContentKind::Secret;
        }
        if (matches(text, link)) {
            return ContentKind::Link;
        }
        if (matches(text, email)) {
            return ContentKind::Email;
        }
        if (matches(text, color)) {
            return ContentKind::Color;
        }
        if (matches(text, path) || matches(text, fileReference)) {
            return ContentKind::Path;
        }
        if (matches(text, command)) {
            return ContentKind::Code;
        }
        if (matches(text, verification)) {
            return ContentKind::VerificationCode;
        }
        if (matches(text, time)) {
            return ContentKind::Time;
        }
        if (matches(text, date)) {
            return ContentKind::Date;
        }
        if (matches(text, phone)) {
            const auto digits = countIf(text, std::isdigit);
            // a lone dot reads as a decimal point; dotted phone numbers have two
            const bool formatted = text.front() == '+' ||
                                   text.find_first_of(" ()-") != std::string::npos ||
                                   std::count(text.begin(), text.end(), '.') >= 2;
            if (digits >= 7 && digits <= 15 && formatted) {
                return ContentKind::Phone;
            }
        }
        if (matches(text, number)) {
            return ContentKind::Number;
        }
    }

    // street addresses run to one line or a few short ones; a house number
    // with a street word, or a postcode, marks them
    if (text.size() < 240 && std::count(text.begin(), text.end(), '\n') < 6) {
        static const std::regex street(
            R"((^|\n)\s*\d+[a-z]?\s+[\w'.-]+(\s+[\w'.-]+)*\s+(street|st|avenue|ave|road|rd|boulevard|blvd|lane|ln|drive|dr|way|court|ct|place|pl|terrace|close|crescent|square|sq|parkway|pkwy|highway|hwy|row|mews|gardens)\b\.?)",
            kIcase);
        static const std::regex postcode(
            R"(\b([a-z]{1,2}\d[a-z\d]?\s?\d[a-z]{2}|[a-z]{2}\s\d{5}(-\d{4})?)\b)", kIcase);
        const bool hasStreet = matches(text, street);
        const bool hasPostcode = matches(text, postcode) &&
                                 (text.find(',') != std::string::npos || !singleLine);
        if (hasStreet || hasPostcode) {
            return ContentKind::Address;
        }
    }

    if (singleLine) {
        if (looksLikeSecret(text)) {
            return ContentKind::Secret;
        }
    }
    return looksLikeCode(text) ? ContentKind::Code : ContentKind::Text;
}

} // namespace kstack::core
