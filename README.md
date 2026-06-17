# qr-basecamp

A **QR code service** for Logos Basecamp: a callable core module that turns any text into a QR
matrix, a standalone **QR Generator** UI, and **drop-in card components** (QML + C++) any module
can embed.

- **`qr` core** — `Q_INVOKABLE` service. Any module calls `logos.callModule("qr","generate",[text])`
  and gets the QR matrix back synchronously. Encoder: vendored [Nayuki QR-Code-generator](https://github.com/nayuki/QR-Code-generator) (C++, MIT) at `src/qrcodegen.*`.
- **`qr_ui`** — the visible **QR Generator** tab (input → Generate → QR + Save as image).
- **`integration/QrCard.qml`** — drop-in QR card for **ui_qml** consumers.
- **`integration/QrBundleCard.{h,cpp}`** — drop-in QR card widget for **Qt-Widgets (`type: ui`)** consumers.

The core returns the QR **matrix as data** (not a PNG): the QML sandbox blocks `data:` URIs, so
consumers render the matrix (a `Grid` of `Rectangle`s, or a rasterized `QImage`).

## Repo layout

```
qr-basecamp/
├── flake.nix · metadata.json · CMakeLists.txt   # the `qr` CORE module (mkLogosModule)
├── src/
│   ├── QrPlugin.{h,cpp}        # the core plugin (generate / generateCard / getLast / savePng)
│   └── qrcodegen.{hpp,cpp}     # vendored Nayuki C++ QR encoder (MIT)
├── plugins/qr_ui/             # the QR Generator UI (mkLogosQmlModule)
│   └── qml/Main.qml
└── integration/               # drop-in components for OTHER modules
    ├── QrCard.qml             #   ui_qml consumers
    ├── QrBundleCard.{h,cpp}   #   Qt-Widgets consumers
    └── README.md              #   the QrCard copy-in guide
```

---

## Install

Builds use `logos-module-builder` (`lgx-portable` target → `linux-amd64` variant) and install with
`lgpm`. Find your `lgpm`:

```bash
lgpm=$(find /nix/store -name lgpm -path "*/logos-package-manager-cli-*/bin/lgpm" | head -1)
MDIR=~/.local/share/Logos/LogosBasecamp/modules
PDIR=~/.local/share/Logos/LogosBasecamp/plugins
```

**1. The `qr` core service** (required for everything):

```bash
cd qr-basecamp
git add -A                                              # nix reads the git tree
nix build .#packages.x86_64-linux.lgx-portable -L
rm -rf "$MDIR/qr"
"$lgpm" --modules-dir "$MDIR" --ui-plugins-dir "$PDIR" --allow-unsigned \
        install --file result/logos-qr-module-lib.lgx
```

**2. The `qr_ui` Generator tab** (optional — the visible UI):

```bash
cd plugins/qr_ui
nix build .#packages.x86_64-linux.lgx-portable -L
rm -rf "$PDIR/qr_ui"
"$lgpm" --modules-dir "$MDIR" --ui-plugins-dir "$PDIR" --allow-unsigned \
        install --file result/logos-qr_ui-module.lgx
```

**3. Clear the QML cache and relaunch:**

```bash
rm -rf ~/.cache/Logos/LogosBasecamp/qmlcache/
kill -9 $(pgrep -f logos_host) 2>/dev/null
~/logos-basecamp-current.AppImage &
```

> **Build target:** use **`lgx-portable`** (variant `linux-amd64`). The plain `lgx` target
> (`linux-amd64-dev`) is rejected by current `lgpm`.

---

## Use

### A. As the QR Generator (standalone)

Open **QR Generator** in the sidebar → type any text/URL → **Generate QR** → **Save as image**
(writes to `~/Pictures/qr/`).

### B. As a service — call it from any module

```qml
// In any ui_qml plugin's QML:
var raw = logos.callModule("qr", "generate", ["https://logos.co"])
var res = JSON.parse(JSON.parse(raw))     // callModule double-encodes; unwrap twice
// res = { ok:true, n:25, cells:[true,false,…(n*n, row-major)], text:"https://logos.co" }
```

Render the matrix (the QML sandbox blocks image `data:` URIs):

```qml
Grid {
    columns: res.n; rows: res.n
    Repeater { model: res.cells
        delegate: Rectangle { width: 6; height: 6; color: modelData ? "#000" : "#fff" } }
}
```

### C. Embed the QR **card** (ui_qml consumers)

Copy `integration/QrCard.qml` into your plugin's `qml/` and use it (full guide:
`integration/README.md`):

```qml
QrCard {
    title:       "My Chat ID"        // optional
    description: "Scan to connect"   // optional
    payload:     someStringToEncode  // ← regenerates on change
    // optional theme: cardBg, titleColor, descColor, accent, qrBg, qrFg …
}
```

It calls `qr.generateCard`, renders title + description + QR, and has a **Save as image** button.
(The property is `payload`, **not** `data` — `data` is a reserved QML `Item` property.)

### D. Embed the QR card **widget** (Qt-Widgets consumers)

Copy `integration/QrBundleCard.{h,cpp}` into your `src/` and feed it the `qr.generateCard` JSON:

```cpp
auto* card = new QrBundleCard(this);
card->setFromQrJson(jsonFromGenerateCard);   // renders title + description + QR + Save button
```

---

## `qr` core API

All methods are **synchronous request/reply** and return a JSON string.

| Method | Returns |
|---|---|
| `generate(text)` | `{ok, n, cells:[bool…(n*n)], text}` or `{ok:false, error}` |
| `generateCard(title, description, data)` | `{ok, title, description, n, cells}` or `{ok:false, error}` |
| `getLast()` | the last generated result, or `{ok:false, error:"none generated"}` |
| `savePng(srcPath, name)` | validates the PNG at `srcPath`, moves it to `~/Pictures/qr/<name>.png` → `{ok, path}` or `{ok:false, error}` |

`cells` is the QR module matrix, **row-major**, length `n*n` (`true` = dark). The Save-as-image
flow is: consumer `grabToImage` → `saveToFile("/tmp/…")` → `qr.savePng(tmp, name)` (file I/O is
C++-only in the sandbox).

---

## Known limitation — loading `qr` from C++-backed UIs

A core only auto-loads when a consumer declares it as a `dependency`. For **pure ui_qml** modules
that's free (`"dependencies": ["qr"]`). But a module **with a C++ backend** that declares `qr`
triggers the typed-SDK codegen, which `qr` (a synchronous service) can't satisfy without becoming
async. Until a **load-only dependency** exists
([logos-module-builder#129](https://github.com/logos-co/logos-module-builder/issues/129)), such a
consumer needs `qr` loaded another way — e.g. open the QR Generator once, or have any pure-QML
module declare `qr`.

## Proven on

The chat module — an intro-bundle QR card:
[logos-co/logos-chat-ui#23](https://github.com/logos-co/logos-chat-ui/pull/23).
