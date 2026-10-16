# Platform support

Capture and encrypted persistence (SQLCipher with Keychain) are built and checked on macOS. Windows and Linux share a Qt capture adapter, with no desktop-session validation and no key-store adapter.

| Session | Capture path | Status |
| --- | --- | --- |
| macOS | AppKit pasteboard poll, 250 ms | Background text, URLs, repeats, and source exit checked; permission denial untested |
| Windows | Qt clipboard signal | Not validated |
| Linux X11 | Qt clipboard signal | Ownership and desktop session not validated |
| Linux Wayland | Qt clipboard signal | Compositor behavior and desktop session not validated |

A session counts as supported once its [P0 checks](../prototypes/p0-clipboard-capture/README.md) pass. The macOS fast-copy trial does not rule out changes missed between polls. Wayland behavior depends on the compositor.

## Desktop integration layer

The desktop app reaches the operating system through two interfaces in `apps/desktop/platform/`, `PlatformUi` and `SystemServices`. Shared code reads their capability flags and does not check which system it runs on. macOS has its own subclasses and factory; other systems use `platform/common/DefaultPlatformFactory.cpp`.

| `SystemServices` capability | Portable base | macOS |
| --- | --- | --- |
| Share | Not offered | System share menu |
| Add calendar event | iCalendar file in the default app | iCalendar file in Calendar |
| Show calendar day | Not offered | Calendar, after Automation consent |
| Reminders | Not offered | Reminders, after access is granted |
| Show on map | OpenStreetMap | Maps |
