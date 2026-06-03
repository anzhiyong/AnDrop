#include "core/TransferManager.h"

#include <QUuid>

#include <utility>

TransferManager::TransferManager(QObject *parent)
    : QObject(parent)
{
}

QList<TransferTask> TransferManager::tasks() const
{
    return m_tasks.values();
}

QString TransferManager::createOutgoingTransfer(qint64 totalBytes)
{
    const QString transferId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    TransferTask task(transferId, totalBytes);
    task.transitionTo(TransferState::WaitingForReceiver);
    m_tasks.insert(transferId, task);
    emitChanged();
    return transferId;
}

void TransferManager::updateProgress(QString transferId, qint64 transferredBytes)
{
    auto it = m_tasks.find(transferId);
    if (it == m_tasks.end()) {
        return;
    }

    if (it->state() == TransferState::Accepted) {
        it->transitionTo(TransferState::Transferring);
    }

    it->setTransferredBytes(transferredBytes);
    emitChanged();
}

void TransferManager::markCompleted(QString transferId)
{
    auto it = m_tasks.find(transferId);
    if (it == m_tasks.end()) {
        return;
    }

    if (it->state() == TransferState::Accepted) {
        it->transitionTo(TransferState::Transferring);
    }
    it->setTransferredBytes(it->totalBytes());
    it->transitionTo(TransferState::Completed);
    emitChanged();
}

void TransferManager::markRejected(QString transferId)
{
    auto it = m_tasks.find(transferId);
    if (it == m_tasks.end()) {
        return;
    }

    it->transitionTo(TransferState::Rejected);
    emitChanged();
}

void TransferManager::markCancelled(QString transferId)
{
    auto it = m_tasks.find(transferId);
    if (it == m_tasks.end()) {
        return;
    }

    it->transitionTo(TransferState::Cancelled);
    emitChanged();
}

void TransferManager::markFailed(QString transferId, QString message)
{
    auto it = m_tasks.find(transferId);
    if (it == m_tasks.end()) {
        return;
    }

    it->fail(std::move(message));
    emitChanged();
}

void TransferManager::emitChanged()
{
    emit tasksChanged(tasks());
}
