/*
 * Author: Runor Ewhro
 * Description: measures how reliably captured text is recognised as code.
 *              Real code (GitHub Linguist's samples) is cut into 1-, 3-, and
 *              10-line windows and should be classified as code; prose
 *              (paragraphs and sentences from .txt books, Markdown documents
 *              without their code blocks, and .chat files of one message per
 *              line) should not be.
 *
 *   kstack_code_kind_eval <linguist>/samples <prose-folder> [show]
 *
 * "show" prints the prose taken for code and the code taken for prose. The
 * samples and prose are read from disk and never added to this repository.
 */
#include <kstack/core/ContentKind.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using kstack::core::ContentKind;

namespace {
std::vector<std::string> readLines(const fs::path& file) {
    std::ifstream in(file);
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

bool blank(const std::string& line) { return line.find_first_not_of(" \t") == std::string::npos; }

bool isComment(const std::string& line) {
    const auto first = line.find_first_not_of(" \t");
    const std::string_view text = std::string_view(line).substr(first == std::string::npos ? 0 : first);
    for (const std::string_view marker : {"//", "/*", "*", "#", "--", ";", "<!--", "%"}) {
        if (text.starts_with(marker) && !text.starts_with("#!") && !text.starts_with("#include")) {
            return true;
        }
    }
    return false;
}

// code windows from the start of the code, a third in, and two thirds in;
// a one-line window is the first substantial line at each of those points
std::vector<std::string> codeWindows(const fs::path& file, std::size_t size) {
    std::vector<std::string> lines;
    for (auto& line : readLines(file)) {
        if (!blank(line)) {
            lines.push_back(std::move(line));
        }
    }
    std::size_t code = 0;
    while (code < lines.size() && isComment(lines[code])) {
        ++code;
    }
    std::vector<std::string> result;
    if (code >= lines.size()) {
        return result;
    }
    const std::size_t rest = lines.size() - code;
    for (std::size_t start : {code, code + rest / 3, code + 2 * rest / 3}) {
        if (size == 1) {
            while (start < lines.size() && (isComment(lines[start]) || lines[start].size() < 12)) {
                ++start;
            }
            if (start < lines.size()) {
                result.push_back(lines[start]);
            }
            continue;
        }
        if (start + size > lines.size() && start != code) {
            continue;
        }
        std::ostringstream window;
        for (std::size_t i = start; i < std::min(start + size, lines.size()); ++i) {
            window << lines[i] << '\n';
        }
        result.push_back(window.str());
    }
    return result;
}

// paragraphs as people copy them from a page: one line each
std::vector<std::string> paragraphs(const std::vector<std::string>& lines) {
    std::vector<std::string> result;
    std::string current;
    for (const auto& line : lines) {
        if (blank(line)) {
            if (!current.empty()) {
                result.push_back(current);
            }
            current.clear();
            continue;
        }
        const auto first = line.find_first_not_of(" \t");
        current += (current.empty() ? "" : " ") + line.substr(first);
    }
    if (!current.empty()) {
        result.push_back(current);
    }
    return result;
}

std::vector<std::string> sentences(const std::string& paragraph) {
    std::vector<std::string> result;
    std::size_t start = 0;
    for (std::size_t i = 0; i + 1 < paragraph.size(); ++i) {
        if ((paragraph[i] == '.' || paragraph[i] == '?' || paragraph[i] == '!') && paragraph[i + 1] == ' ') {
            result.push_back(paragraph.substr(start, i + 1 - start));
            start = i + 2;
        }
    }
    if (start < paragraph.size()) {
        result.push_back(paragraph.substr(start));
    }
    return result;
}

std::vector<std::string> proseUnits(const fs::path& file) {
    std::vector<std::string> lines = readLines(file);
    const std::string extension = file.extension().string();
    std::vector<std::string> units;
    if (extension == ".chat") {
        for (const auto& line : lines) {
            if (!blank(line)) {
                units.push_back(line);
            }
        }
        return units;
    }
    if (extension == ".txt") {
        // Project Gutenberg's licence header and footer are not the book
        std::vector<std::string> book;
        bool inside = false;
        for (const auto& line : lines) {
            if (line.starts_with("*** START")) {
                inside = true;
            } else if (line.starts_with("*** END")) {
                break;
            } else if (inside) {
                book.push_back(line);
            }
        }
        lines = book.empty() ? lines : book;
        // Wikipedia's plain-text export marks headings "== History ==",
        // which text copied from the page does not have
        for (auto& line : lines) {
            if (line.starts_with("==") && line.ends_with("==")) {
                line.clear();
            }
        }
    }
    if (extension == ".md") {
        // the prose of a document, without its code blocks and tables
        std::vector<std::string> prose;
        bool fenced = false;
        for (const auto& line : lines) {
            if (line.starts_with("```")) {
                fenced = !fenced;
                prose.emplace_back();
            } else if (!fenced && !line.starts_with("    ") && !line.starts_with("|")) {
                prose.push_back(line);
            } else {
                prose.emplace_back();
            }
        }
        lines = prose;
    }
    const auto paras = paragraphs(lines);
    // books are long; every few paragraphs is plenty
    const std::size_t step = paras.size() > 400 ? paras.size() / 400 : 1;
    for (std::size_t i = 0; i < paras.size(); i += step) {
        units.push_back(paras[i]);
        for (const auto& sentence : sentences(paras[i])) {
            if (sentence.size() >= 15) {
                units.push_back(sentence);
            }
        }
    }
    return units;
}

std::string percent(int part, int whole) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << (whole ? 100.0 * part / whole : 0) << '%';
    return out.str();
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: kstack_code_kind_eval <linguist>/samples <prose-folder> [show]\n";
        return 2;
    }
    const bool show = argc > 3 && std::string(argv[3]) == "show";

    std::cout << "code recognised as code:\n";
    for (const std::size_t size : {1, 3, 10}) {
        int total = 0, recognised = 0, shown = 0;
        std::map<std::string, std::pair<int, int>> perLanguage;
        for (const auto& folder : fs::directory_iterator(argv[1])) {
            const std::string language = folder.path().filename().string();
            // prose and data formats are not code people expect labelled
            if (!folder.is_directory() || language == "Markdown" || language == "Text") {
                continue;
            }
            for (const auto& file : fs::recursive_directory_iterator(folder.path())) {
                if (!file.is_regular_file()) {
                    continue;
                }
                for (const auto& window : codeWindows(file.path(), size)) {
                    const bool code = kstack::core::classifyContent(window) == ContentKind::Code;
                    ++total;
                    recognised += code;
                    ++perLanguage[language].first;
                    perLanguage[language].second += code;
                    const char* only = std::getenv("KSTACK_EVAL_LANGUAGE");
                    if (show && !code && size == 3 && shown < 25 && (!only || language == only)) {
                        ++shown;
                        std::cout << "  ---- " << language << " taken for something else:\n" << window;
                    }
                }
            }
        }
        std::cout << "  " << std::setw(2) << size << "-line windows: " << percent(recognised, total) << " of "
                  << total;
        if (size == 3) {
            std::cout << "\n    weakest:";
            std::vector<std::pair<double, std::string>> rates;
            for (const auto& [language, counts] : perLanguage) {
                if (counts.first >= 6) {
                    rates.emplace_back(100.0 * counts.second / counts.first, language);
                }
            }
            std::sort(rates.begin(), rates.end());
            for (std::size_t i = 0; i < rates.size() && i < 8; ++i) {
                std::cout << " " << rates[i].second << " " << std::fixed << std::setprecision(0) << rates[i].first
                          << "%";
            }
        }
        std::cout << '\n';
    }

    std::cout << "prose taken for code:\n";
    for (const auto& file : fs::directory_iterator(argv[2])) {
        if (!file.is_regular_file()) {
            continue;
        }
        int total = 0, mistaken = 0, shown = 0;
        for (const auto& unit : proseUnits(file.path())) {
            const bool code = kstack::core::classifyContent(unit) == ContentKind::Code;
            ++total;
            mistaken += code;
            if (show && code && shown < 8) {
                ++shown;
                std::cout << "  ---- " << file.path().filename().string() << ": " << unit.substr(0, 200) << '\n';
            }
        }
        std::cout << "  " << std::left << std::setw(26) << file.path().filename().string() << std::right
                  << percent(mistaken, total) << " of " << total << '\n';
    }
}
