# Architecture

Capture and history logic are independent of Qt and operating-system APIs:

```text
Qt Quick → desktop controllers and models → core services → algorithms
                    │
                    └→ IClipboardMonitor → AppKit or Qt clipboard adapter

HistoryService → EntryRepository → SQLCipher (macOS encrypted build)

capture → TranslationController → CLD2 detection → Bergamot (own thread) → EntrySubtexts
```

The desktop layer owns Qt objects and QML models. `IClipboardMonitor` sends UTF-8 text to the capture pipeline. On macOS, AppKit polls the general pasteboard every 250 ms. Windows and Linux use `QClipboard::dataChanged`, unvalidated on those sessions.

## History and search

With encrypted storage, `HistoryService` loads all saved entries and commits each capture before publishing it. There is no retention limit. `EntryRepository` reuses the id of an exact duplicate and moves it to the top. Content classification is versioned and stored inside SQLCipher. Without encrypted storage, the session holds at most 100 entries.

Search is ASCII case-insensitive substring matching over loaded entries, ranked exact, prefix, then substring. On macOS the database key is in Keychain, and migrations run after SQLCipher verifies it. See the [privacy model](privacy-model.md).

## Derived text

`EntrySubtexts` holds one optional derived string per entry id: a translation, a code language, or a link title.

- **Translation.** `libs/translation` has no Qt dependency. It picks the target from the system's preferred languages, detects the source with CLD2, and routes through one model or two by way of English. Bergamot runs on one background thread and loads each model on first use. Its Marian headers need C++17, so the adapter is a separate target.
- **Code detection.** `ContentClassifier` decides line by line whether text is code. `kstack_code_kind_eval` results: on Linguist's code, 85% of single lines, 90% of 3-line snippets, and 91% of 10-line snippets are recognised. No prose is marked as code on two Project Gutenberg novels, this repository's documents, or chat-style messages; on Wikipedia articles 0–7% is, highest for articles that quote code.
- **Code language.** [codelang](https://github.com/ssjrunor/code-language-detector), fetched at a pinned commit by `cmake/CodeLanguage.cmake`, names the language from a catalog of 836 GitHub Linguist languages and scores 46 of them. `HighlightJsTieBreaker` is its tie-break callback: highlight.js 11.11.1 in Qt's JavaScript engine settles close calls. highlight.js loads on the first close call, in about 40 ms.
