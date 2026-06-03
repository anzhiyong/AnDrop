#pragma once

#include "core/TransferTask.h"

#include <QHash>
#include <QObject>

class TransferManager : public QObject
{
    Q_OBJECT

public:
    explicit TransferManager(QObject *parent = nullptr);

    QList<TransferTask> tasks() const;

public slots:
    QString createOutgoingTransfer(qint64 totalBytes);
    void updateProgress(QString transferId, qint64 transferredBytes);
    void markCompleted(QString transferId);
    void markRejected(QString transferId);
    void markCancelled(QString transferId);
    void markFailed(QString transferId, QString message);

signals:
    void tasksChanged(QList<TransferTask> tasks);

private:
    void emitChanged();

    QHash<QString, TransferTask> m_tasks;
};
