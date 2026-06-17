# Halt — 2026-06-17

## Where we stopped

Just finished **planning the next epic**: created **[EPIC #3]** "Embeddable QR-card component"
in `xAlisher/qr-basecamp` plus 5 sub-issues (#4–#8). Nothing from the epic is implemented yet —
the deliverable this turn was the epic + planned issues, handed back for prioritization.

Everything *before* the epic is **done, verified, committed, and pushed** this session:
- **`qr` core service** (callable QR generator) — `generate(text)→matrix JSON`, `getLast()`,
  `savePng(srcPath,name)→~/Pictures/qr/`. Vendors Nayuki's MIT C++ encoder (`src/qrcodegen.*`).
- **`qr_ui` v0.7.0** — renders via the core (single encoder), **Save-as-image** button, QR centred.
- **`chat-id-showcase`** (separate repo, public) — core+ui stand-in that pushes a Chat ID to `qr_ui`.
- All confirmed working in the GUI by Alisher ("works").

## Current state

- **qr-basecamp**: branch `main`, `21d18e8` "docs(retro): PROJECT_KNOWLEDGE — qr service…". 0 unpushed, clean.
- **chat-id-showcase**: branch `main`, `41cc163` "feat: chat-id-showcase…". Pushed, clean. (github.com/xAlisher/chat-id-showcase, public)
- **basecamp-skills**: branch `opus-4.8/triggers-and-preview`, `0c437bb` "retro(qr-service)…". Pushed, clean.
- **Build status**: all installed + GUI-verified. qr core `qr_plugin.so`, qr_ui 0.7.0, chat_id_showcase installed.
- **Open review**: none.

## Next steps (in order)

1. **Implement the epic — start #4** (`qr` core `generateCard(title, description, data)` → `{ok,title,description,n,cells}`).
   Smallest, logoscore-verifiable. Then #5 (QrCard.qml) ‖ #6 (QrBundleCard C++ widget) → #7 (chat PR) → #8 (styling).
   Build order in the epic: **#4 → (#5 ‖ #6) → #7 → #8**.
2. Per-issue: **logoscore-verify every core method BEFORE wiring UI** (this discipline kept this session crash-free).
3. #7 (chat proof) is the heavy one — clone+build `logos-co/logos-chat-ui` (Qt Widgets + liblogoschat/Waku); save build logs.

## Blockers

- None hard. Three risks flagged in epic #3:
  - **`qr` core reachability from the chat runtime** — chat-ui (separate app) must reach the installed `qr` core over IPC; capability-token bootstrap for a non-UI core caller is unverified (`sdk-capability-token-architecture`).
  - **`logos-chat-ui` build is heavy** (Qt Widgets + liblogoschat). Budget time in #7.
  - **Upstream PR**: `logos-chat-ui` is logos-co — PR may need their sign-off or a fork first.

## Context that's hard to re-derive

- **Two scoping decisions behind the epic (verified, not assumed):**
  1. `logos-chat-ui` is **C++ Qt Widgets** (`type:ui`, QMainWindow/QWidget, no QML) — intro bundle currently
     shows a `QMessageBox` in `ChatWindow::onChatCreateIntroBundleResult` (~line 584). So the card needs **two
     forms**: `QrCard.qml` (ui_qml) + `QrBundleCard` C++ widget (Widgets). Chat proof uses the **C++** form.
  2. Embeddable third-party components must be **vendored (copied in)** — no cross-plugin QML import; mirror
     stash's `integration/StashButton.qml` + `docs/stash-button-integration.md` drop-in pattern. The `qr` *core*
     stays the single generator.
- **chat-ui calls cores** via `m_logos->chat_module.method()` (typed LogosModules) + `.on(event, cb)`; a `qr`
  call would be `m_logos->qr…` or `getClient("qr")->invokeRemoteMethod`. Clone at `~/basecamp/refs/logos-chat-ui`.
- **Platform lessons applied this session (now skills — don't relearn the hard way):**
  - Core emit: **`emit eventResponse(...)` directly, never `getClient(self)->onEventResponse`** (heap-corrupts → std::bad_alloc). The `qr` core sidesteps it by being request/reply (no emit). Skill `core-emit-eventresponse-directly`.
  - **Never `callModule`/`onModuleEvent` an ABSENT module** — blocks the full ~20s IPC timeout (load spinner). Skill `qml-ipc-only-installed-modules`.
  - Save-as-image: `grabToImage → saveToFile("/tmp/…") → C++ savePng` (sandbox blocks data: URIs + QML file I/O). Skill `qml-export-rendered-item-png`.
  - QR centring: Layout child needs `Layout.preferredWidth/Height` + Grid `anchors.centerIn` (raw width/height ignored). Skill `qml-layout-child-no-raw-size`.
- **Build/install/test recipe (operational, easy to forget):**
  - Build: `nix build .#packages.x86_64-linux.lgx-portable` — **NOT `.lgx`** (lgpm 1.0.0 rejects the `-dev` variant).
  - lgpm: `/nix/store/074nr9ih3g965sl2mr7zg2zykkcrhb06-logos-package-manager-cli-1.0.0/bin/lgpm` — `rm -rf` target dir first, `--allow-unsigned install --file …`, then `rm -rf ~/.cache/Logos/LogosBasecamp/qmlcache/`.
  - logoscore: `/nix/store/58wg67723q99kbk1jwiik8ahkxqlnwsq-logos-liblogos-build-0.1.0/bin/logoscore` — copy module to a temp dir, add `-dev` variant keys to manifest `main`, then `-c "mod.method(@file)" --quit-on-finish`. **`@file` passes RAW content — don't quote a path arg.**
  - Relaunch GUI: `setsid bash -c 'DISPLAY=:0 ~/logos-basecamp-current.AppImage >/tmp/x.log 2>&1' &` as a **standalone** bg command — combining it with `pkill` in one command gets the launch reaped (kept failing).
  - Screenshot: **`gnome-screenshot -f`** (grim/scrot return black on this Wayland compositor).
- Issues open: #1 (chat-ID receive, done in spirit), #2 (qr service — v1 landed), #3 epic + #4–#8 subs (not started).
