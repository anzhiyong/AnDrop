#pragma once

#include "protocol/DeviceInfo.h"

#include <QObject>
#include <QStringList>

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);
    void setLocalDeviceId(QString deviceId);

public slots:
    void sendFiles(DeviceInfo target, QStringList filePaths);
    void cancel(QString transferId);

signals:
    void transferProgress(QString transferId, qint64 sentBytes, qint64 totalBytes);
    void transferCompleted(QString transferId);
    void transferFailed(QString transferId, QString message);

private:
    QString m_localDeviceId;
    QString m_cancelledTransferId;
};
