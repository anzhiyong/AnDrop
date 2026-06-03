#pragma once

#include <QString>

enum class TransferState
{
    Pending,
    WaitingForReceiver,
    Accepted,
    Transferring,
    Completed,
    Rejected,
    Cancelled,
    Failed
};

class TransferTask
{
public:
    TransferTask(QString transferId, qint64 totalBytes);

    QString transferId() const;
    TransferState state() const;
    qint64 totalBytes() const;
    qint64 transferredBytes() const;
    int progressPercent() const;
    QString errorMessage() const;

    bool transitionTo(TransferState next);
    void setTransferredBytes(qint64 bytes);
    void fail(QString message);

private:
    bool canTransition(TransferState from, TransferState to) const;

    QString m_transferId;
    TransferState m_state = TransferState::Pending;
    qint64 m_totalBytes = 0;
    qint64 m_transferredBytes = 0;
    QString m_errorMessage;
};
