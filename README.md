# k-stack

This repository contains a local desktop clipboard history app.

The app runs in the background and captures text and links. On macOS, a build with SQLCipher saves history in an encrypted local database with no expiry or count limit. Other builds keep up to 100 entries in memory for the session.

Each entry can carry derived text: an on-device translation, a code snippet's language, or a link's page title. The [roadmap](docs/development-roadmap.md) lists what is unbuilt.

## Build

C++20, Qt 6.4+, CMake 3.24+, and Ninja. On macOS with Homebrew:

```sh
brew install cmake qt ninja sqlcipher
cmake --preset dev -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DKSTACK_PERSISTENCE=ON
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

The output is `build/dev/apps/desktop/kstack_desktop.app` on macOS and `kstack_desktop` elsewhere.

| Option | Effect |
| --- | --- |
| `KSTACK_PERSISTENCE=ON` | Configuration fails without SQLCipher and a platform key store. Without the option, a missing dependency gives session-only history. macOS is the only platform with a key store adapter. |
| `KSTACK_CODESIGN_IDENTITY` | Signs with a stable identity. Keychain ties the history key to the code signature, and the default ad-hoc signature differs on every build, so each rebuild prompts for Keychain access. |
| `KSTACK_ENABLE_TRANSLATION=OFF` | Leaves out Bergamot and CLD2. |

The first configure downloads, at pinned versions: highlight.js 11.11.1 (BSD-3-Clause, SHA-256 checked), [Bergamot](https://github.com/browsermt/bergamot-translator) (MPL-2.0), and [CLD2](https://github.com/CLD2Owners/cld2) (Apache-2.0). Bergamot and CLD2 are about 300 MB of source and add a few minutes to the first build.

Translation models (20–40 MB each, CC-BY-SA-4.0) are not bundled. `scripts/fetch-translation-models.py` downloads them, checks SHA-256, and installs them into `translation-models` in the app's data folder (`~/Library/Application Support/Clipboard History/translation-models` on macOS), or `KSTACK_TRANSLATION_MODELS` if set. The app reads that folder at startup.

## Project notes

- [Architecture](docs/architecture.md)
- [Privacy model](docs/privacy-model.md)
- [Platform support](docs/platform-support.md)
- [P0 capture checks](prototypes/p0-clipboard-capture/README.md)

## License

Copyright (c) 2026 k-stack contributors. The source code is licensed under the [MIT License](LICENSE.md).
