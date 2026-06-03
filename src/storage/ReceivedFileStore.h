#pragma once

#include <QString>

class ReceivedFileStore
{
public:
    explicit ReceivedFileStore(QString downloadDir);

    QString temporaryPath(const QString &transferId, const QString &relativePath, QString *errorMessage = nullptr) const;
    QString finalPathFor(const QString &relativePath, QString *errorMessage = nullptr) const;
    bool completeFile(const QString &temporaryPath, const QString &relativePath, QString *errorMessage = nullptr) const;
    bool cleanupTransfer(const QString &transferId, QString *errorMessage = nullptr) const;

private:
    bool isSafeRelativePath(const QString &relativePath) const;
    QString uniquePath(const QString &path) const;

    QString m_downloadDir;
};
