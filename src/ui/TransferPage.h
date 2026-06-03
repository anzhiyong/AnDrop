#pragma once

#include "core/TransferTask.h"

#include <QWidget>

class QListWidget;

class TransferPage : public QWidget
{
    Q_OBJECT

public:
    explicit TransferPage(QWidget *parent = nullptr);

public slots:
    void setTasks(const QList<TransferTask> &tasks);

signals:
    void cancelRequested(QString transferId);

private:
    QString stateText(TransferState state) const;

    QListWidget *m_taskList = nullptr;
};
