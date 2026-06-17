# QrCard — drop-in QR card for ui_qml modules

Add a titled, described, **savable** QR to any QML module in two steps. The QR is generated
by the shared `qr` core service (you don't bundle an encoder).

## 1 — Copy the component in

```
your-plugin/
└── qml/
    ├── Main.qml
    └── QrCard.qml   ← copy from qr-basecamp/integration/QrCard.qml
```

(Third-party QML components are vendored, not imported — same as stash's `StashButton.qml`.)

## 2 — Declare the `qr` dependency

In your `metadata.json` so the core loads with your plugin:

```json
"dependencies": ["qr"]
```

## 3 — Use it

```qml
QrCard {
    title:       "My Chat ID"
    description: "Scan to start a private chat"
    payload:     someStringToEncode      // ← the data to encode; regenerates on change
}
```

That's it. The card calls `qr.generateCard(title, description, payload)`, renders the
title + description + QR, and shows a **Save as image** button (writes to `~/Pictures/qr/`).

> ⚠️ The property is **`payload`**, not `data` — `data` is a reserved default property on
> every QML `Item`.

## Theming (match your module)

Override any of these to adopt your palette (defaults are dark):

```qml
QrCard {
    payload: id
    cardBg: "#1e1e1e"; titleColor: "#fff"; descColor: "#aaa"
    accent: "#3b82f6"; qrBg: "#fff"; qrFg: "#000"
}
```

## Requirements

- The `qr` core module installed (it ships with `qr-basecamp`).
- The `logos` bridge (standard in any ui_qml plugin). The card degrades gracefully
  (shows an error line) if `qr` isn't reachable.

## Reference consumer

`chat-id-showcase`'s `chat_id_showcase_ui` embeds `QrCard` to show the Chat ID — see
`plugins/chat_id_showcase_ui/qml/Main.qml`.
