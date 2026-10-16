# Development roadmap

## P0 · Clipboard capture

Background capture of text and URLs with timestamps, covering repeats, source-app exit, permissions, and clipboard ownership.

**Status:** macOS [desktop checks](../prototypes/p0-clipboard-capture/README.md) pass for text, URLs, repeats across apps, pause and resume, timestamps, and source exit, and the private-pasteboard test passes. Permission denial is untested: no per-app clipboard setting appeared. Windows, X11, and Wayland are unverified. The 250 ms poll misses an item replaced before the next poll.

**Exit:** Ordinary text capture is reliable on each listed supported session, with limits recorded.

## P1 · Persistence and retrieval

Thousands of items saved, kept across restart, deduplicated by content hash, and searchable.

**Status:** Done on macOS. SQLCipher storage, the automated 5,000-row check, and a 770-copy live run pass; [measurements](../prototypes/p1-storage-search/README.md). Search is substring matching over loaded entries. Memory beyond a few thousand live entries is unmeasured.

**Exit:** Thousands of records survive restart and are searchable under the chosen privacy model.

## P2 · Floating interface

Keyboard-only access to history: global invocation, search, copy, focus return, and window placement.

**Status:** Search, keyboard selection, and copy work. Global invocation, focus return, and placement across desktop sessions are unbuilt.

**Exit:** History can be summoned, searched, and used to copy an item without a mouse.

## P3 · Privacy and platform compatibility

App exclusions, sensitive content, retention, encryption, clipboard permissions, and differences across Windows, macOS, X11, and Wayland.

**Exit:** The [privacy model](privacy-model.md) and [platform matrix](platform-support.md) have test results, with recovery export and platform key handling reviewed before wider release.
