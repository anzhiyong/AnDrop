#include "core/TransferTask.h"

#include <algorithm>
#include <utility>

TransferTask::TransferTask(QString transferId, qint64 totalBytes)
    : m_transferId(std::move(transferId))
    , m_totalBytes(std::max<qint64>(0, totalBytes))
{
}

QString TransferTask::transferId() const
{
    return m_transferId;
}

TransferState TransferTask::state() const
{
    return m_state;
}

qint64 TransferTask::totalBytes() const
{
    return m_totalBytes;
}

qint64 TransferTask::transferredBytes() const
{
    return m_transferredBytes;
}

int TransferTask::progressPercent() const
{
    if (m_totalBytes <= 0) {
        return 0;
    }

    const qint64 clamped = std::clamp(m_transferredBytes, qint64(0), m_totalBytes);
    return static_cast<int>((clamped * 100) / m_totalBytes);
}

QString TransferTask::errorMessage() const
{
    return m_errorMessage;
}

bool TransferTask::transitionTo(TransferState next)
{
    if (!canTransition(m_state, next)) {
        return false;
    }

    m_state = next;
    return true;
}

void TransferTask::setTransferredBytes(qint64 bytes)
{
    m_transferredBytes = std::clamp(bytes, qint64(0), m_totalBytes);
}

void TransferTask::fail(QString message)
{
    m_errorMessage = std::move(message);
    m_state = TransferState::Failed;
}

bool TransferTask::canTransition(TransferState from, TransferState to) const
{
    switch (from) {
    case TransferState::Pending:
        return to == TransferState::WaitingForReceiver || to == TransferState::Cancelled;
    case TransferState::WaitingForReceiver:
        return to == TransferState::Accepted || to == TransferState::Rejected ||
            to == TransferState::Failed || to == TransferState::Cancelled;
    case TransferState::Accepted:
        return to == TransferState::Transferring || to == TransferState::Cancelled;
    case TransferState::Transferring:
        return to == TransferState::Completed || to == TransferState::Cancelled || to == TransferState::Failed;
    case TransferState::Completed:
    case TransferState::Rejected:
    case TransferState::Cancelled:
    case TransferState::Failed:
        return false;
    }

    return false;
}
