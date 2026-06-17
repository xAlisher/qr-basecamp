#include "QrBundleCard.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <Qt>

QrBundleCard::QrBundleCard(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignHCenter);
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    m_titleLabel->setWordWrap(true);

    m_descLabel = new QLabel(this);
    m_descLabel->setAlignment(Qt::AlignHCenter);
    m_descLabel->setStyleSheet("color: #A4A4A4;");
    m_descLabel->setWordWrap(true);

    m_qrLabel = new QLabel(this);
    m_qrLabel->setAlignment(Qt::AlignHCenter);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignHCenter);
    m_statusLabel->setStyleSheet("color: #A4A4A4; font-size: 11px;");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();

    m_saveButton = new QPushButton(tr("Save as image"), this);
    m_saveButton->hide();
    connect(m_saveButton, &QPushButton::clicked, this, &QrBundleCard::onSave);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_descLabel);
    layout->addWidget(m_qrLabel);
    layout->addWidget(m_saveButton);
    layout->addWidget(m_statusLabel);
}

bool QrBundleCard::setFromQrJson(const QString& json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    const QJsonObject o = doc.object();
    if (!o.value("ok").toBool()) {
        m_titleLabel->hide(); m_descLabel->hide(); m_qrLabel->hide(); m_saveButton->hide();
        m_statusLabel->setText(o.value("error").toString(QStringLiteral("QR generation failed")));
        m_statusLabel->setStyleSheet("color: #FB3748; font-size: 11px;");
        m_statusLabel->show();
        return false;
    }
    const int n = o.value("n").toInt();
    const QJsonArray arr = o.value("cells").toArray();
    QVector<bool> cells;
    cells.reserve(arr.size());
    for (const QJsonValue& v : arr) cells.append(v.toBool());
    setCard(o.value("title").toString(), o.value("description").toString(), n, cells);
    return true;
}

void QrBundleCard::setCard(const QString& title, const QString& description,
                           int n, const QVector<bool>& cells)
{
    m_title = title;
    m_statusLabel->hide();

    m_titleLabel->setText(title);
    m_titleLabel->setVisible(!title.isEmpty());
    m_descLabel->setText(description);
    m_descLabel->setVisible(!description.isEmpty());

    if (n > 0 && cells.size() == n * n) {
        const int target = 260;                       // px for the QR area
        const int scale = qMax(1, target / (n + 8));  // +8 for a 4-module quiet zone each side
        const QImage img = matrixToImage(n, cells, scale, 4);
        m_qrLabel->setPixmap(QPixmap::fromImage(img));
        m_qrLabel->show();
        m_saveButton->show();
    } else {
        m_qrLabel->hide();
        m_saveButton->hide();
    }
}

QImage QrBundleCard::matrixToImage(int n, const QVector<bool>& cells, int scale, int quiet)
{
    const int side = (n + 2 * quiet) * scale;
    QImage img(side, side, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
            if (cells[y * n + x])
                p.fillRect((x + quiet) * scale, (y + quiet) * scale, scale, scale, Qt::black);
    p.end();
    return img;
}

QImage QrBundleCard::cardImage() const
{
    // Capture the whole rendered card (title + description + QR) — C++ equivalent of
    // QML's grabToImage. Works because Widgets apps are not sandboxed.
    return const_cast<QrBundleCard*>(this)->grab().toImage();
}

void QrBundleCard::onSave()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (base.isEmpty()) base = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QString dir = base + QStringLiteral("/qr");
    QDir().mkpath(dir);

    QString safe;
    for (const QChar c : m_title)
        if (c.isLetterOrNumber() || c == '-' || c == '_') safe.append(c);
    if (safe.isEmpty()) safe = QStringLiteral("qr");

    const QString suggested = dir + QLatin1Char('/') + safe + QStringLiteral(".png");
    const QString path = QFileDialog::getSaveFileName(this, tr("Save QR as image"),
                                                      suggested, tr("PNG image (*.png)"));
    if (path.isEmpty()) return;

    const bool ok = cardImage().save(path, "PNG");
    m_statusLabel->setText(ok ? tr("Saved: %1").arg(path) : tr("Save failed"));
    m_statusLabel->setStyleSheet(ok ? "color: #22C55E; font-size: 11px;"
                                    : "color: #FB3748; font-size: 11px;");
    m_statusLabel->show();
}
