#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include "interface.h"   // PluginInterface + base-class logosAPI (builder-provided)

// QR generator SERVICE. Any module can call generate(text) over IPC and get back the QR
// module matrix as JSON; the consumer renders it (e.g. a Grid of Rectangles — the QML
// sandbox blocks data: URIs, so we return data, not an image). Request/reply only — no
// events emitted (so no getClient-self emit hazard; see skill core-emit-eventresponse-directly).
class QrPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.logos.QrInterface" FILE "plugin_metadata.json")
    Q_INTERFACES(PluginInterface)

public:
    explicit QrPlugin(QObject* parent = nullptr);

    QString name()    const override { return QStringLiteral("qr"); }
    QString version() const override { return QStringLiteral("0.1.0"); }

    // Called by the shell via reflection — do NOT mark override.
    Q_INVOKABLE void initLogos(LogosAPI* api);

    // Encode `text` to a QR. Returns JSON: {"ok":true,"n":<size>,"cells":[bool…(n*n, row-major)],"text":...}
    // or {"ok":false,"error":...}. Also stored as the "last generated" QR.
    Q_INVOKABLE QString generate(const QString& text);

    // The last generated QR (same JSON shape), or {"ok":false,"error":"none generated"}.
    Q_INVOKABLE QString getLast();

    // Validate the PNG at `srcPath` (e.g. a QML grabToImage temp file) and move it into the
    // user's Pictures/qr dir as <name>.png. File I/O is C++-only (QML sandbox). Removes the
    // source on success. Returns {"ok":true,"path":...} or {"ok":false,"error":...}.
    Q_INVOKABLE QString savePng(const QString& srcPath, const QString& name);

signals:
    // Required — ModuleProxy connects to this on load (eventresponse-signal-required).
    void eventResponse(const QString& eventName, const QVariantList& data);

private:
    QString m_lastJson;
};
