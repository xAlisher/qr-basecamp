#pragma once

#include <QWidget>
#include <QString>
#include <QImage>

class QLabel;
class QPushButton;

// ─────────────────────────────────────────────────────────────────────────────
// QrBundleCard — drop-in QR card WIDGET for Qt-Widgets (type:ui) modules.
//
// A pure renderer: feed it the JSON returned by the `qr` core's generateCard(), and it
// shows title + description + the QR, with a "Save as image" button (QWidget::grab →
// file). The widget does NOT do IPC itself — the host calls the qr core and passes the
// JSON, keeping this widget runtime-agnostic and testable.
//
// Usage (in a Qt-Widgets Logos UI, e.g. logos-chat-ui):
//   auto* card = new QrBundleCard(this);
//   QString json = /* result of qr.generateCard(title, desc, data) over IPC */;
//   card->setFromQrJson(json);
//
// Drop QrBundleCard.{h,cpp} into your module's src/ and add them to your build.
// ─────────────────────────────────────────────────────────────────────────────
class QrBundleCard : public QWidget
{
    Q_OBJECT
public:
    explicit QrBundleCard(QWidget* parent = nullptr);

    // Parse the qr core's generateCard JSON ({ok,title,description,n,cells}) and render.
    // Returns false (and shows an error) on a malformed / error payload.
    bool setFromQrJson(const QString& json);

    // Render directly from a matrix (n×n, row-major dark flags) + metadata.
    void setCard(const QString& title, const QString& description,
                 int n, const QVector<bool>& cells);

    // The composed card as an image (title + description + QR) — used by the save button
    // and available to callers that want the pixels without the file dialog.
    QImage cardImage() const;

private slots:
    void onSave();

private:
    static QImage matrixToImage(int n, const QVector<bool>& cells, int scale, int quiet);

    QLabel*      m_titleLabel = nullptr;
    QLabel*      m_descLabel  = nullptr;
    QLabel*      m_qrLabel    = nullptr;
    QLabel*      m_statusLabel = nullptr;
    QPushButton* m_saveButton = nullptr;

    QString m_title;
};
