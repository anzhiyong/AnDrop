#pragma once

#include <QDialog>

class QLabel;

class ReceiveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveDialog(QWidget *parent = nullptr);
    void setSummary(const QString &senderName, int fileCount, qint64 totalSize, const QString &downloadDir);

private:
    QLabel *m_senderLabel = nullptr;
    QLabel *m_fileCountLabel = nullptr;
    QLabel *m_totalSizeLabel = nullptr;
    QLabel *m_downloadDirLabel = nullptr;
};
