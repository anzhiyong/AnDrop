#include "ui/ReceiveDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ReceiveDialog::ReceiveDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("接收文件"));
    auto *layout = new QVBoxLayout(this);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("接受"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("拒绝"));

    layout->addWidget(new QLabel(QStringLiteral("收到新的文件传输请求"), this));
    m_senderLabel = new QLabel(this);
    m_fileCountLabel = new QLabel(this);
    m_totalSizeLabel = new QLabel(this);
    m_downloadDirLabel = new QLabel(this);
    m_downloadDirLabel->setWordWrap(true);

    layout->addWidget(m_senderLabel);
    layout->addWidget(m_fileCountLabel);
    layout->addWidget(m_totalSizeLabel);
    layout->addWidget(m_downloadDirLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &ReceiveDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ReceiveDialog::reject);
}

void ReceiveDialog::setSummary(const QString &senderName, int fileCount, qint64 totalSize, const QString &downloadDir)
{
    m_senderLabel->setText(QStringLiteral("发送方：%1").arg(senderName));
    m_fileCountLabel->setText(QStringLiteral("文件数量：%1").arg(fileCount));
    m_totalSizeLabel->setText(QStringLiteral("总大小：%1 字节").arg(totalSize));
    m_downloadDirLabel->setText(QStringLiteral("保存目录：%1").arg(downloadDir));
}
