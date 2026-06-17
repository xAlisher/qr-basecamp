#include "QrPlugin.h"
#include "qrcodegen.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include <exception>

using qrcodegen::QrCode;

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
    QJsonObject result;
    if (text.isEmpty()) {
        result["ok"] = false;
        result["error"] = "empty input";
    } else {
        try {
            const QByteArray utf8 = text.toUtf8();
            const QrCode qr = QrCode::encodeText(utf8.constData(), QrCode::Ecc::MEDIUM);
            const int n = qr.getSize();
            QJsonArray cells;
            for (int y = 0; y < n; ++y)
                for (int x = 0; x < n; ++x)
                    cells.append(qr.getModule(x, y));   // row-major
            result["ok"] = true;
            result["n"] = n;
            result["cells"] = cells;
            result["text"] = text;
        } catch (const std::exception& e) {
            result["ok"] = false;
            result["error"] = QString("encode failed (input may be too long): ") + e.what();
        }
    }
    m_lastJson = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
    return m_lastJson;
}

QString QrPlugin::getLast()
{
    if (m_lastJson.isEmpty())
        return QStringLiteral("{\"ok\":false,\"error\":\"none generated\"}");
    return m_lastJson;
}
