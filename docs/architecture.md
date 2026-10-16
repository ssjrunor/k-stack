# Architecture

Capture and history logic stay independent of Qt and operating-system APIs:

```text
Qt Quick → desktop controllers and models → core services → algorithms
                    │
                    └→ IClipboardMonitor → AppKit or Qt clipboard adapter

storage repository → EncryptionProvider → SQLite (not connected to capture)
```

The desktop layer owns Qt objects and updates QML models. `IClipboardMonitor` sends UTF-8 text to the capture pipeline. On macOS, AppKit checks the general pasteboard every 250 ms. Windows and Linux currently use `QClipboard::dataChanged`; those sessions have not been validated.

`HistoryService` keeps up to 100 session entries, timestamps them, and skips consecutive exact duplicates. Search uses ASCII case-insensitive substring matching. The UI starts hidden when a menu bar or tray is available; the history panel has search, keyboard selection, and copy controls. There is no global shortcut yet.

Capture is not connected to `libs/storage`. Its repository requires an `EncryptionProvider`, but no concrete provider exists. The SQL migrations are unapplied sketches. The FTS5 sketch would expose search terms on disk; see the [privacy model](privacy-model.md).