# qr-basecamp

A Logos Basecamp UI module that generates a QR code from any text input.

- Type something in the field, press **Generate QR**, and a scannable QR code
  appears beneath the input.
- Pure QML plugin (`mkLogosQmlModule`) — no core module, no external libraries.
- QR encoding is done in-process by a vendored copy of Kazuhiko Arase's
  [`qrcode-generator`](https://github.com/kazuhikoarase/qrcode-generator) (MIT),
  at `plugins/qr_ui/qml/qrcode.js`. The matrix is rendered with a `Grid` of
  `Rectangle`s (the QML sandbox blocks `data:` URIs, so an image source isn't an
  option).

## Layout

```
plugins/qr_ui/
├── flake.nix          # mkLogosQmlModule
├── metadata.json      # builder config (type: ui_qml)
└── qml/
    ├── Main.qml        # input + Generate button + QR grid
    ├── qrcode.js       # vendored QR encoder (MIT)
    └── icons/icon.png
```

## Build, install, test (dev)

```bash
cd plugins/qr_ui

# 1. all files must be git-tracked (nix reads the git tree)
git add -A

# 2. build the lgx
nix build .#packages.x86_64-linux.lgx -L

# 3. install into the running Basecamp profile
lgpm --modules-dir   ~/.local/share/Logos/LogosBasecamp/modules \
     --ui-plugins-dir ~/.local/share/Logos/LogosBasecamp/plugins \
     --allow-unsigned install --file result/logos-qr_ui-module.lgx

# 4. ui_qml loader expects variant linux-amd64 (lgpm writes -dev); clear qml cache
echo -n "linux-amd64" > ~/.local/share/Logos/LogosBasecamp/plugins/qr_ui/variant
rm -rf ~/.cache/Logos/LogosBasecamp/qmlcache/

# 5. relaunch Basecamp (kill stale logos_host first)
kill -9 $(pgrep -f logos_host) 2>/dev/null
~/logos-basecamp-current.AppImage &
```
