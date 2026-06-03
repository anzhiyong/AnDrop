#include "storage/ReceivedFileStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include <algorithm>
#include <utility>

ReceivedFileStore::ReceivedFileStore(QString downloadDir)
    : m_downloadDir(QDir::cleanPath(std::move(downloadDir)))
{
}

QString ReceivedFileStore::temporaryPath(const QString &transferId, const QString &relativePath, QString *errorMessage) const
{
    if (transferId.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("传输 ID 不能为空");
        }
        return {};
    }

    if (!isSafeRelativePath(relativePath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("接收路径不安全");
        }
        return {};
    }

    const QString cleanRelativePath = QDir::cleanPath(relativePath);
    const QString tempRoot = QDir(m_downloadDir).filePath(QStringLiteral(".androp_tmp/%1").arg(transferId));
    const QString path = QDir(tempRoot).filePath(cleanRelativePath + QStringLiteral(".part"));
    const QFileInfo info(path);

    QDir dir;
    if (!dir.mkpath(info.dir().absolutePath())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法创建临时文件目录");
        }
        return {};
    }

    return QDir::cleanPath(path);
}

QString ReceivedFileStore::finalPathFor(const QString &relativePath, QString *errorMessage) const
{
    if (!isSafeRelativePath(relativePath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("接收路径不安全");
        }
        return {};
    }

    const QString cleanRelativePath = QDir::cleanPath(relativePath);
    const QString target = QDir(m_downloadDir).filePath(cleanRelativePath);
    const QFileInfo info(target);

    QDir dir;
    if (!dir.mkpath(info.dir().absolutePath())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法创建目标文件目录");
        }
        return {};
    }

    return uniquePath(QDir::cleanPath(target));
}

bool ReceivedFileStore::completeFile(const QString &temporaryPath, const QString &relativePath, QString *errorMessage) const
{
    const QString target = finalPathFor(relativePath, errorMessage);
    if (target.isEmpty()) {
        return false;
    }

    if (!QFileInfo::exists(temporaryPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("临时文件不存在");
        }
        return false;
    }

    if (!QFile::rename(temporaryPath, target)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法移动接收文件到目标位置");
        }
        return false;
    }

    return true;
}

bool ReceivedFileStore::cleanupTransfer(const QString &transferId, QString *errorMessage) const
{
    const QString tempRoot = QDir(m_downloadDir).filePath(QStringLiteral(".androp_tmp/%1").arg(transferId));
    QDir dir(tempRoot);
    if (!dir.exists()) {
        return true;
    }

    if (!dir.removeRecursively()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法清理临时文件");
        }
        return false;
    }

    return true;
}

bool ReceivedFileStore::isSafeRelativePath(const QString &relativePath) const
{
    if (relativePath.trimmed().isEmpty()) {
        return false;
    }

    const QFileInfo info(relativePath);
    if (info.isAbsolute()) {
        return false;
    }

    const QString cleanPath = QDir::cleanPath(relativePath);
    const QStringList parts = cleanPath.split('/', Qt::SkipEmptyParts);
    return std::none_of(parts.begin(), parts.end(), [](const QString &part) {
        return part == QStringLiteral("..");
    });
}

QString ReceivedFileStore::uniquePath(const QString &path) const
{
    if (!QFileInfo::exists(path)) {
        return path;
    }

    const QFileInfo info(path);
    const QString dir = info.dir().absolutePath();
    const QString baseName = info.completeBaseName();
    const QString suffix = info.suffix();

    for (int index = 1; index < 10000; ++index) {
        const QString candidateName = suffix.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(baseName).arg(index)
            : QStringLiteral("%1 (%2).%3").arg(baseName).arg(index).arg(suffix);
        const QString candidate = QDir(dir).filePath(candidateName);
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return path;
}
