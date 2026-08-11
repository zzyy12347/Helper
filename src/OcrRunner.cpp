#include "OcrRunner.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

namespace {
QString makeTempImagePath()
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/itemshopfinder-ocr";
    QDir().mkpath(baseDir);
    return baseDir + QString("/capture-%1.png").arg(QDateTime::currentMSecsSinceEpoch());
}
}

OcrRunner::OcrRunner(QObject *parent)
    : QObject(parent)
{
}

QString OcrRunner::runtimeRoot() const
{
    const QString appRoot = QCoreApplication::applicationDirPath() + "/third_party/rapidocr";
    if (QFileInfo::exists(appRoot)) {
        return QDir(appRoot).absolutePath();
    }

    const QString sourceRoot = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../third_party/rapidocr/runtime");
    if (QFileInfo::exists(sourceRoot)) {
        return QDir(sourceRoot).absolutePath();
    }

    return appRoot;
}

QString OcrRunner::workerPath() const
{
    return runtimeRoot() + "/rapidocr_worker/rapidocr_worker.exe";
}

bool OcrRunner::isAvailable(QString *errorMessage) const
{
    if (!QFileInfo::exists(workerPath())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("未找到离线 OCR 组件：%1").arg(QDir::toNativeSeparators(workerPath()));
        }
        return false;
    }

    return true;
}

OcrResult OcrRunner::recognize(const QImage &image) const
{
    OcrResult result;
    QString availabilityError;
    if (!isAvailable(&availabilityError)) {
        result.errorMessage = availabilityError;
        return result;
    }

    const QString imagePath = makeTempImagePath();
    if (!image.save(imagePath)) {
        result.errorMessage = QStringLiteral("截图保存失败，无法送入 OCR。");
        return result;
    }

    QProcess process;
    process.setWorkingDirectory(runtimeRoot() + "/rapidocr_worker");
    process.start(workerPath(), {QStringLiteral("--image"), QDir::toNativeSeparators(imagePath)});
    if (!process.waitForStarted(5000)) {
        QFile::remove(imagePath);
        result.errorMessage = QStringLiteral("离线 OCR 进程启动失败。");
        return result;
    }

    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished(3000);
        QFile::remove(imagePath);
        result.errorMessage = QStringLiteral("离线 OCR 超时，没有返回结果。");
        return result;
    }

    QFile::remove(imagePath);

    const QByteArray stdOut = process.readAllStandardOutput();
    const QByteArray stdErr = process.readAllStandardError();
    const QJsonDocument document = QJsonDocument::fromJson(stdOut);
    if (!document.isObject()) {
        result.errorMessage = QStringLiteral("离线 OCR 返回了无法解析的数据。%1")
                                  .arg(QString::fromUtf8(stdErr).trimmed());
        return result;
    }

    const QJsonObject object = document.object();
    result.success = object.value("ok").toBool(false);
    result.text = object.value("text").toString();
    result.averageScore = object.value("averageScore").toDouble(0.0);
    for (const QJsonValue &value : object.value("lines").toArray()) {
        result.lines.push_back(value.toString());
    }
    if (!result.success) {
        result.errorMessage = object.value("error").toString(QString::fromUtf8(stdErr).trimmed());
    }

    return result;
}
