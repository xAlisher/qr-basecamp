# qr-basecamp — Project Knowledge

Accumulated project wisdom. Platform-wide lessons live in `~/basecamp/basecamp-skills/`.

## Architecture

- Pure QML `ui_qml` plugin (`mkLogosQmlModule`) — no core C++ module, no external libs.
- QR **encoding**: vendored copy of Kazuhiko Arase's [`qrcode-generator`](https://github.com/kazuhikoarase/qrcode-generator)
  (MIT) at `plugins/qr_ui/qml/qrcode.js`. Imported as `import "qrcode.js" as QR`.
  - `var qr = QR.qrcode(0, 'M')` — **typeNumber `0` = auto-fit**, ECC level `M`.
  - `qr.addData(text); qr.make();` then read `qr.getModuleCount()` and `qr.isDark(r,c)`.
  - The library's UMD tail (`typeof define` / `typeof exports` guards) is harmless in
    QML's JS engine — no shim needed.
- QR **rendering**: `Grid` + `Repeater` of `Rectangle`s over a flat boolean array
  (`qrCells`, length N²). NOT an `Image` with a `data:` URI — the sandbox blocks `data:`
  URIs and network. See skill `qml-render-generated-bitmap-rectangle-grid`.

## Build / install (dev)

```bash
cd plugins/qr_ui
git add -A                                         # nix reads the git tree
nix build .#packages.x86_64-linux.lgx-portable -L  # NOT .lgx — see below
lgpm --modules-dir   ~/.local/share/Logos/LogosBasecamp/modules \
     --ui-plugins-dir ~/.local/share/Logos/LogosBasecamp/plugins \
     --allow-unsigned install --file result/logos-qr_ui-module.lgx
rm -rf ~/.cache/Logos/LogosBasecamp/qmlcache/      # stale cache → "Type unavailable"
# relaunch Basecamp (kill stale logos_host first)
```

## Pitfalls hit

- **Use `lgx-portable`, not `lgx`.** Current `lgpm` v1.0.0 *rejects* the plain `.lgx`
  target (variant `linux-amd64-dev`) — *"Package does not contain variant for platform:
  linux-x86_64"* — and *accepts* `.lgx-portable` (variant `linux-amd64`). This is the
  opposite of what the older skills documented; lgpm's variant acceptance flipped.
  Tracked in `builder-lgx-install-recipe` (updated 2026-06-16). No post-install variant
  reset needed — `lgx-portable` writes `linux-amd64` directly.
- Verifying a visual feature: `grim`/`scrot` capture **black** under this Wayland
  compositor (no `wlr-screencopy`); `gnome-screenshot -f` works via the portal.

## Chat ID integration (qr_ui ← chat_id_showcase)

`qr_ui` receives a Chat ID (intro bundle) from another module and renders its QR. It is a
**pure push receiver** (v0.5.1): it only `logos.onModuleEvent("chat_id_showcase",
"chatCreateIntroBundleResult")` and draws the QR when the event arrives. The companion
[`chat-id-showcase`](https://github.com/xAlisher/chat-id-showcase) core+ui pushes the ID via
`setIntroBundle` + `createIntroBundle`.

Three hard-won rules (each cost a debug cycle — all now platform skills):
- **Core emit:** `emit eventResponse(...)` **directly**. The first version used
  `logosAPI->getClient("chat_id_showcase")->onEventResponse(this,…)` → `std::bad_alloc`
  crash the instant `createIntroBundle` was called. (skill: `core-emit-eventresponse-directly`)
- **Never touch an absent module from QML.** `qr_ui` used to also `callModule`/`onModuleEvent`
  on `chat_module` (not installed) → each blocked the **full ~20s IPC timeout** → load spinner.
  `onModuleEvent` is *not* free for missing modules. (skill: `qml-ipc-only-installed-modules`)
- **No blocking IPC in `Component.onCompleted`.** A sync call there freezes the first frame
  while the core cold-spawns. Pure push (subscribe only) avoids it entirely.
- Debugging: **`logoscore` headless** (`-c "chat_id_showcase.createIntroBundle()"`) reproduced
  the core crash in seconds and let me bisect emit-removed → direct-emit without GUI cycles.
