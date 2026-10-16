# Privacy model

## Current behavior

The process keeps at most 100 captured items in memory. Quitting clears that list; it does not clear the system clipboard. The menu bar or tray and history panel can pause capture or clear the session.

Text and URL formats are captured as text. Other formats are ignored. Consecutive exact copies produce one entry; other repeats can enter history. There are no app exclusions, secret detection, network sync, telemetry, or persistent history yet. Copied passwords or private text can therefore enter the session list.

On macOS, a 250 ms poll can miss an item replaced before the next check. The monitor checks an explicit always-deny pasteboard setting when it starts; permission changes while running are not yet verified. The setting was not visible during P0 desktop checks. [Apple describes when per-app pasteboard settings appear](https://developer.apple.com/documentation/appkit/nspasteboard/accessbehavior-swift.enum?language=objc).

## Before persistent storage

Capture will remain separate from SQLite until authenticated encryption, key storage and recovery, failure handling, and a search design that avoids plaintext terms on disk are in place. `libs/storage` has a provider interface but no implementation. The FTS5 migration is an unapplied sketch; applying it to encrypted records as written would expose search terms. Stored content hashes also need a keyed design for short or predictable text.

Deleting a database row cannot guarantee physical erasure from WAL files, backups, snapshots, or SSDs. Clearing the app's history also leaves any current system clipboard content in place.
