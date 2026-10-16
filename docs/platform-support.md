# Platform support

Capture has been built and checked on macOS. Windows and Linux have a shared Qt adapter but no desktop-session validation yet.

| Session | Capture path | Status |
| --- | --- | --- |
| macOS | AppKit pasteboard poll, 250 ms | Background text, URLs, repeats, and source exit checked; permission denial untested |
| Windows | Qt clipboard signal | Desktop validation pending |
| Linux X11 | Qt clipboard signal | Ownership and desktop validation pending |
| Linux Wayland | Qt clipboard signal | Compositor behavior and desktop validation pending |

Sessions are listed as supported after their [P0 checks](../prototypes/p0-clipboard-capture/README.md) pass. The successful fast-copy trial on macOS does not rule out missed changes between polls. Wayland support depends on the compositor.
