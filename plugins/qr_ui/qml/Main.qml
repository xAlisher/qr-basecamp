import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root

    // ── Palette (aligned with stash/notes/keycard) ────────────────────────
    readonly property color bgPrimary:     "#171717"
    readonly property color bgSecondary:   "#262626"
    readonly property color textPrimary:   "#FFFFFF"
    readonly property color textSecondary: "#A4A4A4"
    readonly property color textMuted:     "#5D5D5D"
    readonly property color accentOrange:  "#FF5000"
    readonly property color errorRed:      "#FB3748"
    readonly property color successGreen:  "#22C55E"
    readonly property color borderColor:   "#383838"

    // ── QR state ──────────────────────────────────────────────────────────
    property int    qrSize:  0      // module count (N) of the rendered matrix
    property var    qrCells: []     // flat row-major array of booleans, length N*N
    property string errorMsg: ""
    property string saveMsg:  ""
    property bool   saveOk:   false

    // Save the rendered QR as a PNG. Grabs the on-screen frame to a temp file (QML side),
    // then hands it to the qr core's savePng to validate + move into place (file I/O is
    // C++-only). Mirrors ia-basecamp's grabToImage → saveToFile → finalize pattern.
    function saveImage() {
        saveMsg = ""
        if (qrSize <= 0) return
        if (typeof logos === "undefined") { saveOk = false; saveMsg = "Bridge unavailable."; return }
        var name = "qr-" + Qt.formatDateTime(new Date(), "yyyyMMdd-hhmmss")
        var tmp = "/tmp/" + name + ".png"
        qrFrame.grabToImage(function(result) {
            if (!result.saveToFile(tmp)) {
                root.saveOk = false; root.saveMsg = "Could not capture the QR image."; return
            }
            var res = root.callModuleParse(logos.callModule("qr", "savePng", [tmp, name]))
            if (res && res.ok) { root.saveOk = true;  root.saveMsg = "Saved: " + res.path }
            else               { root.saveOk = false; root.saveMsg = "Save failed: " + (res && res.error ? res.error : "qr service unavailable") }
        })
    }

    // ── Chat ID (intro bundle) — pure receiver ────────────────────────────
    // Source that PUSHES a Chat ID via the createIntroBundle/chatCreateIntroBundleResult
    // contract. Only subscribe to modules that are actually installed: onModuleEvent has to
    // connect to the module's source, so subscribing to an ABSENT module blocks the full
    // ~20s IPC timeout. (To also receive the real chat_module, add it here only when it ships.)
    readonly property var chatSources: ["chat_id_showcase"]

    Component.onCompleted: subscribeTimer.start()

    // Defer subscription off onCompleted so the view paints immediately — onModuleEvent can
    // block briefly while the core's source comes up.
    Timer {
        id: subscribeTimer
        interval: 200
        repeat: false
        onTriggered: {
            if (typeof logos === "undefined") return
            for (var i = 0; i < chatSources.length; i++)
                logos.onModuleEvent(chatSources[i], "chatCreateIntroBundleResult")
        }
    }

    // chatCreateIntroBundleResult payload: [bool success, int status, QString bundle, QString ts]
    Connections {
        target: typeof logos !== "undefined" ? logos : null
        function onModuleEventReceived(moduleName, eventName, data) {
            if (eventName !== "chatCreateIntroBundleResult") return
            if (root.chatSources.indexOf(moduleName) < 0) return
            var success = data && (data[0] === true || data[0] === "true")
            var bundle  = data ? (data[2] || "") : ""
            if (success && bundle.length > 0) {
                input.text = bundle           // input shows the Chat ID
                root.generate(bundle)         // generate the QR
            }
        }
    }

    // Build the QR matrix by calling the shared `qr` core service. Returns true on success.
    // (Synchronous callModule — fine on the user-initiated Generate; the qr core is light and
    // spawns fast. The core is the single encoder; we just render its matrix.)
    function generate(text) {
        errorMsg = ""
        qrSize = 0
        qrCells = []
        if (!text || text.length === 0) {
            errorMsg = "Enter some text first."
            return false
        }
        if (typeof logos === "undefined") {
            errorMsg = "Module bridge unavailable (standalone preview)."
            return false
        }
        var res = callModuleParse(logos.callModule("qr", "generate", [text]))
        if (!res || !res.ok) {
            errorMsg = (res && res.error) ? res.error
                     : "QR service unavailable — is the 'qr' module installed?"
            return false
        }
        qrSize = res.n
        qrCells = res.cells          // flat row-major bool array, length n*n
        return true
    }

    // logos.callModule returns a double-JSON-encoded string; unwrap to an object.
    function callModuleParse(raw) {
        try {
            var v = JSON.parse(raw)
            if (typeof v === "string") v = JSON.parse(v)
            return v
        } catch (e) { return null }
    }

    Rectangle {
        anchors.fill: parent
        color: bgPrimary

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 64, 420)
            spacing: 20

            Label {
                text: "QR Generator"
                color: textPrimary
                font.pixelSize: 22
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Type any data and generate a scannable QR code."
                color: textSecondary
                font.pixelSize: 13
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            // ── Input ─────────────────────────────────────────────────────
            TextField {
                id: input
                Layout.fillWidth: true
                placeholderText: "Enter text, URL, anything…"
                color: textPrimary
                placeholderTextColor: textMuted
                font.pixelSize: 15
                padding: 12
                selectByMouse: true
                background: Rectangle {
                    color: bgSecondary
                    radius: 8
                    border.color: input.activeFocus ? accentOrange : borderColor
                    border.width: 1
                }
                onAccepted: root.generate(text)
            }

            // ── Generate button ───────────────────────────────────────────
            Button {
                id: genButton
                text: "Generate QR"
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                onClicked: root.generate(input.text)
                contentItem: Text {
                    text: genButton.text
                    color: textPrimary
                    font.pixelSize: 15
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: genButton.pressed ? "#CC4000"
                         : genButton.hovered ? "#FF6420" : accentOrange
                }
            }

            // ── Error message ─────────────────────────────────────────────
            Label {
                text: errorMsg
                visible: errorMsg.length > 0
                color: errorRed
                font.pixelSize: 12
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            // ── QR output ─────────────────────────────────────────────────
            // Fixed white square; the matrix is CENTRED inside it (anchors.centerIn),
            // so the quiet-zone margin is symmetric on all four sides.
            Rectangle {
                id: qrFrame
                visible: qrSize > 0
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 320
                Layout.preferredHeight: 320
                radius: 8
                color: "#FFFFFF"

                // Largest cell that leaves ≥16px quiet on each side.
                readonly property int cell: qrSize > 0 ? Math.max(1, Math.floor((width - 32) / qrSize)) : 1

                Grid {
                    id: grid
                    anchors.centerIn: parent
                    columns: qrSize
                    rows: qrSize
                    Repeater {
                        model: qrCells
                        delegate: Rectangle {
                            width:  qrFrame.cell
                            height: qrFrame.cell
                            color: modelData ? "#000000" : "#FFFFFF"
                        }
                    }
                }
            }

            // ── Save as image ─────────────────────────────────────────────
            Button {
                id: saveButton
                text: "Save as image"
                visible: qrSize > 0
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                onClicked: root.saveImage()
                contentItem: Text {
                    text: saveButton.text
                    color: textPrimary
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: "transparent"
                    border.color: saveButton.hovered ? accentOrange : borderColor
                    border.width: 1
                }
            }

            Label {
                text: saveMsg
                visible: saveMsg.length > 0
                color: saveOk ? successGreen : errorRed
                font.pixelSize: 12
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WrapAnywhere
            }
        }
    }
}
