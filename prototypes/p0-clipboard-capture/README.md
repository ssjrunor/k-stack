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
| Two different copies less than 250 ms apart | Both appeared in one trial; another timing could miss one |
| Deny and restore clipboard permission | Untested; no per-app setting appeared |

The private-pasteboard test also passed in a normal Terminal. It checks text, URL-only data, repeats, timestamps, pause, and capture after its writer exits without changing the system clipboard:

```sh
ctest --preset dev -R mac_clipboard --output-on-failure
```

It skips inside the agent sandbox when the pasteboard service is unavailable. [Apple says](https://developer.apple.com/documentation/appkit/nspasteboard/accessbehavior-swift.enum?language=objc) a per-app setting may not appear until an access alert occurs. Permission denial and changes while capture is already running remain unverified. Windows, X11, and Wayland checks are still pending in the [platform matrix](../../docs/platform-support.md).
