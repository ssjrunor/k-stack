# Development roadmap

## P0 · Clipboard capture

Capture background text and URLs with timestamps. Check repeats, source-app exit, permissions, and clipboard ownership.

**Current:** The macOS build runs in the background. [Desktop checks](../prototypes/p0-clipboard-capture/README.md) cover text, URLs, repeats across apps, pause and resume, timestamps, and source exit. The private-pasteboard test passes. Permission denial could not be tested because no per-app clipboard setting appeared. Windows, X11, and Wayland remain unverified. A 250 ms poll can miss intermediate rapid copies.

**Exit:** Ordinary text capture is reliable on each listed supported session, with limits recorded.

## P1 · Persistence and retrieval

Save thousands of items, retain them after restart, deduplicate with content hashes, and compare SQLite FTS5 with substring search.

**Current:** SQLite code and migrations exist but are not connected to capture. Encrypted storage is required before saving real clipboard history. The FTS5 sketch is unsafe to apply to encrypted records as written.

**Exit:** Thousands of records survive restart and remain searchable under the chosen privacy model.

## P2 · Floating interface

Finish the small search panel, keyboard flow, global shortcut, focus return, and window placement.

**Current:** The QML panel opens from the menu bar or tray and has search, keyboard selection, and copy controls. Global invocation and focus return remain.

**Exit:** The panel can be summoned, searched, and used to copy an item without a mouse.

## P3 · Privacy and platform compatibility

Evaluate app exclusions, sensitive content, retention, encryption, clipboard permissions, and differences across Windows, macOS, X11, and Wayland.

**Exit:** The [privacy model](privacy-model.md) and [platform matrix](platform-support.md) have test results, and an encrypted-storage approach is ready before production persistence.
