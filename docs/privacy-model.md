# Privacy model

## Current behavior

On macOS, a build with SQLCipher saves captured items in an encrypted database and loads them after restart. Saved items do not expire. Removing an item or clearing history deletes its database row. Pausing or quitting leaves saved history in place. A build without SQLCipher keeps at most 100 items in memory and discards them on quit. Neither mode clears the system clipboard.

Translations run on the device with installed models and are held in memory only. Removing an entry, clearing history, or quitting discards its translation. The app downloads no models; `scripts/fetch-translation-models.py` does, when run by hand.

Copying an `http`, `https`, or `www.` link makes the app request that page once to read its title. The request sends no cookies, reads at most 256 KB, stops at the title, and times out after 8 seconds. The app resolves each hostname itself and connects only to a checked public IP address, checks every redirect the same way, and bypasses any network proxy. Local and reserved addresses and nonstandard web ports are excluded. The site sees the request, the user's IP address, and the full URL, including any token in it; a single-use link, such as a sign-in link, is spent by the request. The title is held in memory only. Title requests are always on.

Text and URL formats are captured as text. Other formats are ignored. The encrypted repository reuses a row for exact duplicate content, comparing text after the hash so collisions never merge different items. Session-only builds skip consecutive exact copies. There are no app exclusions, secret detection, network sync, or telemetry, so copied passwords and private text enter history.

On macOS, the 250 ms poll misses an item replaced before the next poll. Where AppKit exposes pasteboard access behavior, the monitor checks for an explicit always-deny setting at startup and on every poll, updates capture status when it changes, and resumes when access returns. Individual denials under the Ask setting are indistinguishable. Permission changes are unverified on a desktop: the setting did not appear during P0 checks. [Apple describes when per-app pasteboard settings appear](https://developer.apple.com/documentation/appkit/nspasteboard/accessbehavior-swift.enum?language=objc).

## Stored history

SQLCipher encrypts the database and WAL page data. The random database key is in the macOS Keychain, outside the database. A missing or wrong key leaves the existing file untouched. A save failure during capture stops capture and reports the failure. The database holds original text, timestamps, a content hash, a pin flag, and versioned content classification. Translations and link titles are not stored.

There is no recovery export: losing the Keychain key makes the file unreadable, and a copied database file alone does not open on another computer. Search reads decrypted entries into process memory. Filesystem metadata such as the file's existence and size is visible. Software running in the unlocked account has access to the app's memory and Keychain item.

Deleting a database row does not guarantee physical erasure from WAL files, backups, snapshots, or SSDs. Clearing history leaves the current system clipboard content in place.
