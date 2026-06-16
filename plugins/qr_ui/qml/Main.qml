import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "qrcode.js" as QR

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
    readonly property color borderColor:   "#383838"

    // ── QR state ──────────────────────────────────────────────────────────
    property int    qrSize:  0      // module count (N) of the rendered matrix
    property var    qrCells: []     // flat row-major array of booleans, length N*N
    property string errorMsg: ""

    // Build the QR module matrix from `text`. Returns true on success.
    function generate(text) {
        errorMsg = ""
        qrSize = 0
        qrCells = []
        if (!text || text.length === 0) {
            errorMsg = "Enter some text first."
            return false
        }
        try {
            var qr = QR.qrcode(0, 'M')   // type 0 = auto-fit, ECC level M
            qr.addData(text)
            qr.make()
            var n = qr.getModuleCount()
            var cells = new Array(n * n)
            for (var r = 0; r < n; r++)
                for (var c = 0; c < n; c++)
                    cells[r * n + c] = qr.isDark(r, c)
            qrSize = n
            qrCells = cells
            return true
        } catch (e) {
            errorMsg = "Could not encode: " + (e.message || e) + " (input may be too long)."
            return false
        }
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
            Rectangle {
                id: qrFrame
                visible: qrSize > 0
                Layout.alignment: Qt.AlignHCenter
                // white quiet-zone border around the matrix
                readonly property int boxSize: 300
                readonly property int cell: qrSize > 0 ? Math.floor(boxSize / qrSize) : 1
                readonly property int matrix: cell * qrSize
                readonly property int quiet: cell * 4
                width:  matrix + quiet * 2
                height: matrix + quiet * 2
                radius: 8
                color: "#FFFFFF"

                Grid {
                    id: grid
                    x: qrFrame.quiet
                    y: qrFrame.quiet
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
        }
    }
}
