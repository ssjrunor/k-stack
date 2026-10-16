# P1 persistence and retrieval

The macOS encrypted build saves captures in SQLCipher. It loads all rows after restart, reuses an existing row for exact duplicate content, and deletes rows only on request. `encrypted_history` checks 5,000 synthetic entries, core search after restart, duplicates, delete, clear, unreadable ciphertext, wrong-key rejection, and migration of an older database.

## Measurements

From the 5,000-row test on this Mac:

| Measure | Result |
| --- | --- |
| 200 exact-token FTS5 queries | about 1 ms |
| 200 SQL `LIKE` substring queries | about 80 ms |
| Core restart load | about 4 ms |
| Saved-item substring search | about 2 ms |

FTS5 matches tokens and `LIKE` matches substrings, so the two timings are not like for like. Memory beyond a few thousand live entries is unmeasured.

## Live macOS stress run

770 copies at the default interval were all saved, with no errors or missed copies. After relaunch, history loaded without a noticeable delay, search found the items, and a deleted item stayed deleted across another relaunch. Memory footprint was about 81–93 MB, peaking near 97 MB. The 0.3-second minimum interval follows the 250 ms poll; the automated 5,000-row test covers scale.

`scripts/stress-clipboard.py` produced the run. It copies synthetic prose, code, localhost links, colors, numbers, and multiline text through `pbcopy` into the running app and checks each item in its encrypted database. It requires a Debug build with `KSTACK_ENABLE_STRESS_PROFILE=ON`; release builds exclude stress support. `--isolated` uses its own database and Keychain item.
