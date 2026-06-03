#include "ui/TransferPage.h"

#include <QListWidget>
#include <QVBoxLayout>

TransferPage::TransferPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    m_taskList = new QListWidget(this);
    layout->addWidget(m_taskList, 1);
}

void TransferPage::setTasks(const QList<TransferTask> &tasks)
{
    m_taskList->clear();

    for (const TransferTask &task : tasks) {
        m_taskList->addItem(QStringLiteral("%1  %2  %3%")
            .arg(task.transferId(), stateText(task.state()))
            .arg(task.progressPercent()));
    }
}

QString TransferPage::stateText(TransferState state) const
{
    switch (state) {
    case TransferState::Pending:
        return QStringLiteral("等待中");
    case TransferState::WaitingForReceiver:
        return QStringLiteral("等待接收方确认");
    case TransferState::Accepted:
        return QStringLiteral("已接受");
    case TransferState::Transferring:
        return QStringLiteral("传输中");
    case TransferState::Completed:
        return QStringLiteral("已完成");
    case TransferState::Rejected:
        return QStringLiteral("已拒绝");
    case TransferState::Cancelled:
        return QStringLiteral("已取消");
    case TransferState::Failed:
        return QStringLiteral("失败");
    }

    return QStringLiteral("未知");
}
