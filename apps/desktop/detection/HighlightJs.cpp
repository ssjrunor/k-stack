/*
 * Author: Runor Ewhro
 * Description: loads highlight.js from the app's resources into a private
 *              JavaScript engine the first time it is needed, for settling
 *              close calls on a snippet's language and for colouring code in
 *              the preview. Two gaps in Qt's engine are bridged on the way
 *              in; see engineShim and the loader.
 */
#include "HighlightJs.hpp"

#include <QFile>
#include <QHash>
#include <QJSEngine>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QStringList>

namespace kstack::desktop {
namespace {

const QStringList& scripts() {
    static const QStringList files{
        "highlight.min.js",          "languages/cmake.min.js",  "languages/dart.min.js",
        "languages/dockerfile.min.js", "languages/dos.min.js",  "languages/elixir.min.js",
        "languages/haskell.min.js",  "languages/powershell.min.js", "languages/scala.min.js",
    };
    return files;
}

// runs before highlight.js. Qt's engine has no console, and its regular
// expressions reject Unicode property escapes such as \p{L}, which the
// Python, XML, and Haskell grammars use; a pattern Qt rejects is retried
// with each escape spelled out as ranges covering the main scripts, which is
// close enough for telling languages apart
constexpr auto engineShim = R"js(
var console = { log() {}, warn() {}, error() {}, info() {} };
(function () {
  var letters = 'A-Za-z\u00AA\u00B5\u00BA\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u02C1\u0370-\u03FF' +
    '\u0400-\u0481\u048A-\u052F\u0531-\u0556\u05D0-\u05EA\u0620-\u064A\u0904-\u0939' +
    '\u0E01-\u0E30\u1100-\u11FF\u3041-\u3096\u30A1-\u30FA\u4E00-\u9FFF\uAC00-\uD7A3';
  var classes = {
    L: letters,
    XID_Start: letters,
    XID_Continue: letters + '0-9_\u0300-\u036F\u0660-\u0669',
    N: '0-9\u00B2\u00B3\u00B9\u00BC-\u00BE\u0660-\u0669',
    S: '$+<=>^`|~\u00A2-\u00A9\u00AC\u00AE-\u00B1\u00B4\u00B8\u00D7\u00F7\u2100-\u214F' +
       '\u2190-\u23FF\u2500-\u27BF',
    P: '!-#%-*,-/:;?@\\[-\\]_{}\u00A1\u00A7\u00AB\u00B6\u00B7\u00BB\u00BF\u2010-\u2027' +
       '\u2030-\u205E\u3001-\u3003'
  };
  function spellOut(pattern) {
    var out = '', inClass = false;
    for (var i = 0; i < pattern.length; i++) {
      var c = pattern[i];
      if (c === '\\' && pattern[i + 1] === 'p' && pattern[i + 2] === '{') {
        var end = pattern.indexOf('}', i);
        var ranges = classes[pattern.slice(i + 3, end)];
        if (ranges === undefined) return null;
        out += inClass ? ranges : '[' + ranges + ']';
        i = end;
      } else if (c === '\\') {
        out += c + (pattern[i + 1] || '');
        i++;
      } else {
        if (c === '[') inClass = true;
        else if (c === ']') inClass = false;
        out += c;
      }
    }
    return out;
  }
  var NativeRegExp = RegExp;
  RegExp = function (pattern, flags) {
    try {
      return new NativeRegExp(pattern, flags);
    } catch (error) {
      var source = typeof pattern === 'string' ? pattern : pattern.source;
      var spelled = source.indexOf('\\p{') < 0 ? null : spellOut(source);
      if (spelled === null) throw error;
      return new NativeRegExp(spelled, flags);
    }
  };
  RegExp.prototype = NativeRegExp.prototype;
})();
)js";

} // namespace

HighlightJs::HighlightJs() = default;
HighlightJs::~HighlightJs() = default;

QString HighlightJs::nameFor(std::string_view linguistName) {
    static const QHash<QString, QString> names{
        {"Shell", "bash"},       {"PowerShell", "powershell"}, {"Batchfile", "dos"},
        {"Python", "python"},    {"Ruby", "ruby"},             {"Perl", "perl"},
        {"PHP", "php"},          {"JavaScript", "javascript"}, {"TypeScript", "typescript"},
        {"Java", "java"},        {"Kotlin", "kotlin"},         {"Scala", "scala"},
        {"C#", "csharp"},        {"C", "c"},                   {"C++", "cpp"},
        {"Objective-C", "objectivec"}, {"Swift", "swift"},     {"Rust", "rust"},
        {"Go", "go"},            {"Dart", "dart"},             {"Lua", "lua"},
        {"R", "r"},              {"SQL", "sql"},               {"HTML", "xml"},
        {"XML", "xml"},          {"SVG", "xml"},          {"TSX", "typescript"},          {"CSS", "css"},               {"SCSS", "scss"},
        {"YAML", "yaml"},        {"TOML", "ini"},              {"INI", "ini"},
        {"Markdown", "markdown"}, {"Dockerfile", "dockerfile"}, {"Makefile", "makefile"},
        {"CMake", "cmake"},      {"Diff", "diff"},             {"Haskell", "haskell"},
        {"Elixir", "elixir"},    {"JSON", "json"},
    };
    return names.value(QString::fromUtf8(linguistName.data(), static_cast<qsizetype>(linguistName.size())));
}

bool HighlightJs::ready() {
    if (engine_ || failed_) {
        return !failed_;
    }
    // the minified scripts reference names before declaring them, which is
    // valid but makes Qt log a warning for each one
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.usedbeforedeclared=false"));
    engine_ = std::make_unique<QJSEngine>();
    failed_ = engine_->evaluate(QString::fromUtf8(engineShim)).isError();
    for (const QString& script : scripts()) {
        QFile file(QStringLiteral(":/highlightjs/") + script);
        if (failed_ || !file.open(QIODevice::ReadOnly)) {
            failed_ = true;
            break;
        }
        // Qt's engine treats "__proto__: null" in an object literal as a
        // property holding null rather than as a null prototype, so
        // highlight.js would find a null among its modes and languages;
        // without it the objects get the default prototype, which adds no
        // enumerable keys
        QString source = QString::fromUtf8(file.readAll());
        source.remove(QRegularExpression(QStringLiteral("__proto__\\s*:\\s*null\\s*,?")));
        const QJSValue result = engine_->evaluate(source, file.fileName());
        if (result.isError()) {
            failed_ = true;
            break;
        }
    }
    if (!failed_) {
        const QJSValue hljs = engine_->globalObject().property("hljs");
        highlightAuto_ = hljs.property("highlightAuto");
        highlight_ = hljs.property("highlight");
        failed_ = !highlightAuto_.isCallable() || !highlight_.isCallable();
    }
    if (failed_) {
        engine_.reset();
    }
    return !failed_;
}

std::optional<HighlightJs::Detection> HighlightJs::detect(const QString& text, const QString& first,
                                                        const QString& second) {
    if (!ready()) {
        return std::nullopt;
    }
    QJSValue subset = engine_->newArray(2);
    subset.setProperty(0, first);
    subset.setProperty(1, second);
    const QJSValue result = highlightAuto_.call({text, subset});
    if (result.isError()) {
        return std::nullopt;
    }
    const QJSValue secondBest = result.property("secondBest");
    return Detection{result.property("language").toString(),
                     result.property("relevance").toInt() -
                         (secondBest.isObject() ? secondBest.property("relevance").toInt() : 0)};
}

QString HighlightJs::highlight(const QString& text, const QString& language) {
    if (language.isEmpty() || !ready()) {
        return {};
    }
    QJSValue options = engine_->newObject();
    options.setProperty("language", language);
    // a snippet cut mid-construct still gets its colours
    options.setProperty("ignoreIllegals", true);
    const QJSValue result = highlight_.call({text, options});
    return result.isError() ? QString() : result.property("value").toString();
}

} // namespace kstack::desktop
