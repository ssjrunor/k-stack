/*
 * Author: Runor Ewhro
 * Description: checks that highlight.js loads from the resources, settles
 *              close calls between two candidate languages, and colours code
 *              for the preview. With
 *              KSTACK_TIEBREAK_EVAL set to a dump written by codelang's
 *              codelang_eval (CODELANG_EVAL_DUMP), it also measures how many close
 *              calls it names right and wrong, as the app would.
 */
#include "detection/CodeHighlighter.hpp"
#include "detection/HighlightJsTieBreaker.hpp"

#include <QColor>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
using codelang::detectCodeLanguage;
using codelang::findLanguage;
using kstack::desktop::CodeHighlighter;
using kstack::desktop::HighlightJs;
using kstack::desktop::HighlightJsTieBreaker;

int failures = 0;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

void checkChoices(HighlightJsTieBreaker& breaker) {
    const auto& python = *findLanguage("Python");
    const auto& ruby = *findLanguage("Ruby");
    const char* pythonCode = "import os\n\ndef list_python_files(folder):\n"
                             "    \"\"\"Return the Python files in a folder.\"\"\"\n    found = []\n"
                             "    for name in os.listdir(folder):\n"
                             "        if name.endswith('.py') and not name.startswith('_'):\n"
                             "            found.append(name)\n    return found\n";
    require(breaker.choose(pythonCode, ruby, python) == &python, "Python not chosen over Ruby");
    // highlight.js leans to Python here, but only by 2, short of the lead
    // it needs, so the call stays open
    const char* shortPython = "import os\n\nfor name in os.listdir('.'):\n    if name.endswith('.py'):\n"
                              "        print(name)\n";
    require(breaker.choose(shortPython, ruby, python) == nullptr, "chose with too small a lead");
    require(breaker.choose(pythonCode, python, ruby) == &python, "choice depends on argument order");
    const char* rubyCode = "class Greeter\n  def initialize(name)\n    @name = name\n  end\n\n"
                           "  def greet\n    puts \"Hello #{@name}\"\n  end\nend\n";
    require(breaker.choose(rubyCode, python, ruby) == &ruby, "Ruby not chosen over Python");
    // highlight.js names both "xml", so it cannot choose between them
    require(breaker.choose("<a>x</a>", *findLanguage("HTML"), *findLanguage("XML")) == nullptr,
            "chose between languages it cannot tell apart");
    // a language highlight.js does not know is never chosen
    require(breaker.choose("x", python, *findLanguage("Clojure")) == nullptr, "unknown language chosen");
    // no evidence either way leaves the call open
    require(breaker.choose("x", python, ruby) == nullptr, "chose without a clear lead");
}

void checkRendering(CodeHighlighter& highlighter) {
    const QVariantMap palette{{"font", "Menlo"},          {"structure", QColor("#112233")},
                              {"value", QColor("#445566")}, {"comment", QColor("#778899")},
                              {"added", QColor("#00aa00")}, {"removed", QColor("#aa0000")}};
    const auto python = highlighter.render("def greet(name):\n\treturn f\"Hi {name}\"  # say hi\n", "Python", palette);
    const QString html = python.value("html").toString();
    require(html.contains("color:#112233") && html.contains(">def<"), "keyword not coloured as structure");
    require(html.contains("color:#445566"), "string not coloured as a value");
    require(html.contains("color:#778899;font-style:italic"), "comment not faded");
    require(html.contains("font-weight:600"), "function name not set in weight");
    require(html.contains("    <span") && !html.contains("\t"), "tab not expanded to four columns");
    require(!html.contains("class=\""), "highlight.js classes left in the output");
    require(python.value("lines").toInt() == 2 && python.value("gutter").toString().contains("1\n2"),
            "line count or gutter wrong");
    const auto unknown = highlighter.render("a < b && c", "Code", palette);
    require(unknown.value("html").toString().contains("a &lt; b &amp;&amp; c"), "unknown language not escaped");
    QString longCode;
    for (int i = 0; i < CodeHighlighter::maximumLines + 20; ++i) {
        longCode += "x = 1\n";
    }
    const auto longOne = highlighter.render(longCode, "Python", palette);
    require(longOne.value("lines").toInt() == CodeHighlighter::maximumLines + 20 &&
                longOne.value("shownLines").toInt() == CodeHighlighter::maximumLines,
            "long paste not cut to its start");
}

// the close calls in a dump, settled as the app settles them
void measure(HighlightJsTieBreaker& breaker, const char* path) {
    QFile file(QString::fromLocal8Bit(path));
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "cannot read " << path << '\n';
        ++failures;
        return;
    }
    int total = 0, settled = 0, right = 0, wrong = 0;
    const auto choose = [&breaker](std::string_view text, const auto& first, const auto& second) {
        return breaker.choose(text, first, second);
    };
    while (!file.atEnd()) {
        const QJsonObject row = QJsonDocument::fromJson(file.readLine()).object();
        ++total;
        if (!row.value("guess").toString().isEmpty()) {
            continue;
        }
        const std::string snippet = row.value("snippet").toString().toStdString();
        const auto guess = detectCodeLanguage(snippet, choose);
        if (!guess) {
            continue;
        }
        ++settled;
        const std::string expected = row.value("expected").toString().toStdString();
        const std::string_view named = guess->language->name;
        const bool close = (expected == "TypeScript" && named == "JavaScript") ||
                           ((expected == "C++" || expected == "Objective-C") && named == "C") ||
                           (expected == "SCSS" && named == "CSS");
        if (named == expected) {
            ++right;
        } else if (!close) {
            ++wrong;
        }
    }
    std::cout << total << " snippets: the tie-breaker named " << settled << ", adding " << right << " right ("
              << 100.0 * right / total << "%) and " << wrong << " wrong (" << 100.0 * wrong / total << "%)\n";
}
} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    HighlightJs highlightJs;
    HighlightJsTieBreaker breaker(highlightJs);
    CodeHighlighter highlighter(highlightJs);
    checkChoices(breaker);
    checkRendering(highlighter);
    if (const char* dump = std::getenv("KSTACK_TIEBREAK_EVAL")) {
        measure(breaker, dump);
    }
    if (failures > 0) {
        std::cerr << failures << " highlight.js checks failed\n";
        return 1;
    }
}
