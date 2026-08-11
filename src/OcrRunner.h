#pragma once

#include <QImage>
#include <QObject>
#include <QString>
#include <QStringList>

struct OcrResult {
    bool success = false;
    QString text;
    QStringList lines;
    double averageScore = 0.0;
    QString errorMessage;
};

class OcrRunner : public QObject {
    Q_OBJECT

public:
    explicit OcrRunner(QObject *parent = nullptr);

    bool isAvailable(QString *errorMessage = nullptr) const;
    OcrResult recognize(const QImage &image) const;
    QString workerPath() const;

private:
    QString runtimeRoot() const;
};
