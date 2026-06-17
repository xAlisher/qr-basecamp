#include "QrPlugin.h"
#include "qrcodegen.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <exception>

using qrcodegen::QrCode;

namespace {
QString jsonOut(const QJsonObject& o) {
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

// Encode `data` to a QR matrix. On success sets ok/n/cells; on failure sets ok=false/error.
// Shared by generate() and generateCard().
QJsonObject encodeMatrix(const QString& data) {
    QJsonObject r;
    if (data.isEmpty()) {
        r["ok"] = false; r["error"] = "empty input"; return r;
    }
    try {
        const QByteArray utf8 = data.toUtf8();
        const QrCode qr = QrCode::encodeText(utf8.constData(), QrCode::Ecc::MEDIUM);
        const int n = qr.getSize();
        QJsonArray cells;
        for (int y = 0; y < n; ++y)
            for (int x = 0; x < n; ++x)
                cells.append(qr.getModule(x, y));   // row-major
        r["ok"] = true; r["n"] = n; r["cells"] = cells;
    } catch (const std::exception& e) {
        r["ok"] = false;
        r["error"] = QString("encode failed (input may be too long): ") + e.what();
    }
    return r;
}
}

QrPlugin::QrPlugin(QObject* parent)
    : QObject(parent)
{
    qDebug() << "QrPlugin constructed";
}

void QrPlugin::initLogos(LogosAPI* api)
{
    // Store in the PluginInterface base-class member (the shell checks it).
    logosAPI = api;
    qDebug() << "QrPlugin: Logos API initialized";
}

QString QrPlugin::generate(const QString& text)
{
    QJsonObject result = encodeMatrix(text);
    if (result["ok"].toBool())
        result["text"] = text;
    m_lastJson = jsonOut(result);
    return m_lastJson;
}

QString QrPlugin::generateCard(const QString& title, const QString& description, const QString& data)
{
    QJsonObject result = encodeMatrix(data);
    if (result["ok"].toBool()) {
        result["title"] = title;
        result["description"] = description;
    }
    m_lastJson = jsonOut(result);
    return m_lastJson;
}

QString QrPlugin::getLast()
{
    if (m_lastJson.isEmpty())
        return QStringLiteral("{\"ok\":false,\"error\":\"none generated\"}");
    return m_lastJson;
}

QString QrPlugin::savePng(const QString& srcPath, const QString& name)
{
    QJsonObject r;

    QFileInfo srcInfo(srcPath);
    if (!srcInfo.exists() || !srcInfo.isFile()) {
        r["ok"] = false; r["error"] = "source not found"; return jsonOut(r);
    }

    // Validate it's a real PNG (magic) before trusting the grabbed file.
    QFile src(srcPath);
    if (!src.open(QIODevice::ReadOnly)) {
        r["ok"] = false; r["error"] = "cannot read source"; return jsonOut(r);
    }
    const QByteArray head = src.read(8);
    src.close();
    static const QByteArray pngMagic = QByteArray::fromHex("89504e470d0a1a0a");
    if (!head.startsWith(pngMagic)) {
        r["ok"] = false; r["error"] = "not a valid PNG"; return jsonOut(r);
    }

    // Sanitize the file name (alnum, '-', '_').
    QString safe;
    for (const QChar c : name)
        if (c.isLetterOrNumber() || c == '-' || c == '_') safe.append(c);
    if (safe.isEmpty()) safe = QStringLiteral("qr");

    QString base = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (base.isEmpty()) base = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString dir = base + QStringLiteral("/qr");
    if (!QDir().mkpath(dir)) {
        r["ok"] = false; r["error"] = "cannot create destination dir"; return jsonOut(r);
    }

    const QString dest = dir + QLatin1Char('/') + safe + QStringLiteral(".png");
    QFile::remove(dest);
    if (!QFile::copy(srcPath, dest)) {
        r["ok"] = false; r["error"] = "cannot write file"; return jsonOut(r);
    }
    QFile::remove(srcPath);   // clean up the temp grab

    r["ok"] = true; r["path"] = dest;
    return jsonOut(r);
}
