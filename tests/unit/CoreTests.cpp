/*
 * Author: Runor Ewhro
 * Description: checks core capture, classification, search, and storage contracts.
 */
#include <kstack/algorithms/ContentHasher.hpp>
#include <kstack/algorithms/SearchRanker.hpp>
#include <kstack/core/ContentKind.hpp>
#include <kstack/core/HistoryService.hpp>
#include <kstack/core/SearchService.hpp>

#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
void checkClassification() {
    using kstack::core::ContentKind;
    using kstack::core::classifyContent;
    struct Case {
        const char* text;
        ContentKind kind;
    };
    const Case cases[] = {
        {"https://example.com/documentation/guide", ContentKind::Link},
        {"www.example.com/path", ContentKind::Link},
        {"  rewhro@example.com\n", ContentKind::Email},
        {"+44 20 7946 0958", ContentKind::Phone},
        {"(555) 123-4567", ContentKind::Phone},
        {"221B Baker Street, London NW1 6XE", ContentKind::Address},
        {"1 Infinite Loop\nCupertino, CA 95014", ContentKind::Address},
        {"#7BCBBD", ContentKind::Color},
        {"rgb(123, 203, 189)", ContentKind::Color},
        {"hsl(170 43% 64%)", ContentKind::Color},
        {"cmake --build --preset dev", ContentKind::Code},
        {"int main() {\n    return 0;\n}", ContentKind::Code},
        {"{\"a\": 1}", ContentKind::Code},
        {"~/projects/k-stack/README.md", ContentKind::Path},
        {"/usr/local/bin", ContentKind::Path},
        {"(apps/desktop/controllers/LinkTitleController.cpp:31", ContentKind::Path},
        {"src/main.rs:12:5", ContentKind::Path},
        {"LinkTitleController.cpp:31", ContentKind::Path},
        {"/Users/sam/project/app.py:7", ContentKind::Path},
        {"Foo::bar(1)", ContentKind::Code},
        {"example.com/docs/guide.html", ContentKind::Text},
        {"Tr0ub4dor&3x!", ContentKind::Secret},
        {"node->next2", ContentKind::Code},
        {"1,234,567.89", ContentKind::Number},
        {"$49.99", ContentKind::Number},
        {"42%", ContentKind::Number},
        {"1234567.89", ContentKind::Number},
        {"2026-10-16", ContentKind::Date},
        {"Thursday, 12 November 2026", ContentKind::Date},
        {"Nov 12, 2026", ContentKind::Date},
        {"14:30", ContentKind::Time},
        {"2:30 pm", ContentKind::Time},
        {"482913", ContentKind::VerificationCode},
        {"482 913", ContentKind::VerificationCode},
        {"G-482913", ContentKind::VerificationCode},
        {"Tr0ub4dor&3x", ContentKind::Secret},
        {"sk-live1234567890abcdef", ContentKind::Secret},
        {"-----BEGIN OPENSSH PRIVATE KEY-----\nabc\n-----END OPENSSH PRIVATE KEY-----",
         ContentKind::Secret},
        {"SELECT id, name\nFROM users\nWHERE active = 1;", ContentKind::Code},
        {"SELECT id, name FROM users WHERE active = 1;", ContentKind::Code},
        {"CREATE TABLE entries (\n  id INTEGER PRIMARY KEY,\n  text TEXT NOT NULL\n);", ContentKind::Code},
        {"[package]\nname = \"kstack\"\nversion = \"0.1.0\"", ContentKind::Code},
        {"name: CI\non:\n  push:\njobs:\n  build:\n    runs-on: ubuntu-24.04", ContentKind::Code},
        {"\"isDnD\":false,\n\"name\":\"VCT\",\n\"parent\":{", ContentKind::Code},
        {"from thrift.Thrift import TType, TMessageType, TException", ContentKind::Code},
        {"import (\n\t\"fmt\"\n\t\"os\"\n)", ContentKind::Code},
        {"I came; I saw; I copied it twice.", ContentKind::Text},
        {"Select all the files from the folder and delete the old ones.", ContentKind::Text},
        {"Note: use the --force option only if nothing else works.", ContentKind::Text},
        {"Let me know what you think => I'm leaning towards the blue one.", ContentKind::Text},
        {"See the [privacy model](privacy-model.md) before you change `libs/storage`.", ContentKind::Text},
        {"In Python, == compares two objects by value, and is compares their identity.", ContentKind::Text},
        {"[Illustration]", ContentKind::Text},
        {"Hi Sam,\nThanks for the notes; I'll read them tonight.\nSee you Friday!", ContentKind::Text},
        {"Can we move the review to Thursday at 2?", ContentKind::Text},
        {"kstack_desktop", ContentKind::Text},
        {"camelCaseName", ContentKind::Text},
        {"", ContentKind::Text},
    };
    for (const auto& item : cases) {
        if (classifyContent(item.text) != item.kind) {
            std::cerr << "classification failed for: " << item.text << '\n';
            std::exit(1);
        }
    }
    kstack::core::HistoryService history;
    require(history.add("#7BCBBD").has_value() &&
                history.entries().front().kind == ContentKind::Color,
            "capture did not record the kind");
    require(history.add("fn main() {\n    let mut total = 0;\n    println!(\"{}\", total);\n}").has_value() &&
                history.entries().front().codeLanguage == "Rust",
            "capture did not record the code language");
    require(history.add("#!/usr/bin/env python3\nprint('hi')").has_value() &&
                history.entries().front().codeLanguage == "Python",
            "shebang language not recorded");
    require(history.add("Can we move the review to Thursday at 2?").has_value() &&
                history.entries().front().codeLanguage.empty(),
            "prose given a code language");
}

class MemoryStore final : public kstack::core::IHistoryStore {
public:
    std::vector<kstack::core::ClipboardEntry> rows;
    bool fail = false;
    std::uint64_t nextId = 1;

    std::vector<kstack::core::ClipboardEntry> loadAll() override { return rows; }
    std::uint64_t record(const kstack::core::ClipboardEntry& entry) override {
        if (fail) throw std::runtime_error("write failed");
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].text == entry.text) {
                auto row = rows[i];
                row.capturedAt = entry.capturedAt;
                rows.erase(rows.begin() + static_cast<std::ptrdiff_t>(i));
                rows.insert(rows.begin(), row);
                return row.id;
            }
        }
        auto row = entry;
        row.id = nextId++;
        rows.insert(rows.begin(), row);
        return row.id;
    }
    void updateMetadata(std::span<const kstack::core::ClipboardEntry> entries) override {
        for (const auto& entry : entries) {
            for (auto& row : rows) {
                if (row.id == entry.id) {
                    row.kind = entry.kind;
                    row.codeLanguage = entry.codeLanguage;
                    row.metadataReady = true;
                }
            }
        }
    }
    void remove(std::uint64_t id) override {
        if (fail) throw std::runtime_error("delete failed");
        std::erase_if(rows, [id](const auto& row) { return row.id == id; });
    }
    void clear() override {
        if (fail) throw std::runtime_error("clear failed");
        rows.clear();
    }
};

void checkPersistedHistoryContract() {
    MemoryStore store;
    kstack::core::HistoryService history(kstack::core::RetentionPolicy{2});
    history.attachStore(store);
    for (int i = 0; i < 120; ++i) {
        require(history.add("entry " + std::to_string(i)).has_value(), "persistent capture failed");
    }
    require(history.entries().size() == 120, "persistent history was pruned");
    const auto oldId = history.entries()[119].id;
    require(history.add("entry 0").has_value(), "older duplicate was not moved forward");
    require(history.entries().size() == 120 && history.entries().front().id == oldId,
            "duplicate did not reuse its stored id");
    store.fail = true;
    bool saveRejected = false;
    try {
        (void)history.add("failed entry");
    } catch (const std::runtime_error&) {
        saveRejected = true;
    }
    require(saveRejected, "failed save did not throw");
    require(history.entries().size() == 120, "failed save changed visible history");
    bool clearRejected = false;
    try {
        history.clear();
    } catch (const std::runtime_error&) {
        clearRejected = true;
    }
    require(clearRejected, "failed clear did not throw");
    require(history.entries().size() == 120, "failed clear changed visible history");
    store.fail = false;
    kstack::core::HistoryService restarted;
    restarted.attachStore(store);
    require(restarted.entries().size() == 120 && restarted.entries().front().id == oldId,
            "stored history did not reload");
    require(restarted.remove(oldId), "stored delete failed");
    require(restarted.entries().size() == 119 && store.rows.size() == 119,
            "delete did not reach the store");
    restarted.clear();
    require(restarted.entries().empty() && store.rows.empty(), "clear did not reach the store");
}

void checkSearchOrdering() {
    kstack::core::HistoryService history(kstack::core::RetentionPolicy{10});
    (void)history.add("prefix TEST");
    (void)history.add("test suffix");
    (void)history.add("TEST");
    (void)history.add("another test match");
    (void)history.add("test again");
    const auto matches = kstack::core::SearchService(history).search(" TEST ");
    require(matches.size() == 5, "search lost a substring match");
    require(matches[0].text == "TEST" && matches[1].text == "test again" &&
                matches[2].text == "test suffix" &&
                matches[3].text == "another test match" && matches[4].text == "prefix TEST",
            "search rank or capture order changed");
}
} // namespace

int main() {
    kstack::core::HistoryService history(kstack::core::RetentionPolicy{2});
    const auto beforeCapture = std::chrono::system_clock::now();
    require(history.add("First URL: https://example.com").has_value(), "first capture failed");
    const auto afterCapture = std::chrono::system_clock::now();
    require(history.entries().front().capturedAt >= beforeCapture &&
                history.entries().front().capturedAt <= afterCapture,
            "capture timestamp failed");
    require(!history.add("First URL: https://example.com").has_value(), "duplicate captured");
    require(history.add("Second text").has_value(), "second capture failed");
    require(history.add("Third text").has_value(), "third capture failed");
    require(history.entries().size() == 2, "retention limit failed");
    require(history.entries().front().text == "Third text", "newest item order failed");
    require(kstack::core::SearchService(history).search("SECOND").size() == 1,
            "case-insensitive search failed");
    require(kstack::core::SearchService(history).search("First").empty(), "evicted item found");
    require(kstack::algorithms::searchScore("Hello world", "hello") == 2,
            "prefix ranking failed");
    require(kstack::algorithms::contentHash("a") != kstack::algorithms::contentHash("b"),
            "content fingerprint failed");
    checkClassification();
    checkPersistedHistoryContract();
    checkSearchOrdering();
}
