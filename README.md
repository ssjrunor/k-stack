# k-stack

This repository contains a local, privacy-focused desktop clipboard history app.

The current build runs in the background, captures text and links, and keeps the latest 100 entries in memory. The menu bar or tray opens history, pauses capture, and clears the session. The history panel supports search and copying an earlier item. Quitting clears the session. Captured text is not written to the database yet.

## Build

The build requires C++20, Qt 6.4+, CMake 3.24+, Ninja, and SQLite development files. On macOS with Homebrew:

```sh
brew install cmake qt ninja sqlite
cmake --preset dev -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build --preset dev
ctest --preset dev --output-on-failure
open build/dev/apps/desktop/kstack_desktop.app
```

On other systems, install the same dependencies and run the CMake commands without `CMAKE_PREFIX_PATH` when Qt is in a standard location. The executable is `kstack_desktop`; macOS builds an app bundle.

The history panel opens from the menu bar or tray and includes keyboard search and selection controls. A global shortcut, login startup, persistent history, encrypted storage, and installers are still planned. See the [roadmap](docs/development-roadmap.md).

## Project notes

- [Architecture](docs/architecture.md)
- [Privacy model](docs/privacy-model.md)
- [Platform support](docs/platform-support.md)
- [P0 capture checks](prototypes/p0-clipboard-capture/README.md)

## License

Copyright (c) 2026 k-stack contributors. The source code is licensed under the [MIT License](LICENSE.md).
