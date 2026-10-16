# P0 capture checks

The macOS desktop build was checked on 2026-10-15. These are live clipboard observations:

| Check | Result |
| --- | --- |
| Text from another app while history was closed | Captured |
| Browser URL, then close the browser | URL stayed in history |
| Same text copied twice, including across apps | One entry |
| Pause, resume, and clear | Worked |
| Text written by a `pbcopy` process that exited | Captured |
| Timestamps and several ordinary copies | Matched the session |
| Two different copies less than 250 ms apart | Both captured in one trial; an item replaced before the next poll is missed |
| Deny and restore clipboard permission | Untested; no per-app setting appeared |

The `mac_clipboard` test passes in a normal Terminal. It uses a private pasteboard to check text, URL-only data, repeats, timestamps, pause, and capture after the writer exits, without touching the system clipboard. It skips where no pasteboard service is available, such as the agent sandbox. [Apple documents](https://developer.apple.com/documentation/appkit/nspasteboard/accessbehavior-swift.enum?language=objc) when the per-app setting appears. The monitor reports explicit always-deny changes during capture and resumes when access returns; those transitions are unverified on a desktop. Windows, X11, and Wayland are unchecked; see the [platform matrix](../../docs/platform-support.md).
