# Contributing

Contributions are licensed under the [MIT License](../LICENSE.md).

`libs/core` and `libs/algorithms` stay free of Qt and OS APIs. Platform code belongs behind the interfaces in `libs/platform`. Clipboard capture must stay separate from persistent storage until the [encryption plan](privacy-model.md) is implemented.

Before proposing a change, run:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

For clipboard reports, include the OS, desktop session, build or commit, steps, and result. Use synthetic content; copied secrets do not belong in logs, fixtures, screenshots, or issues.
