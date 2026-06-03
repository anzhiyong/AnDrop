#pragma once

#include "protocol/DeviceInfo.h"

#include <QHash>
#include <QObject>

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(QString localDeviceId, QObject *parent = nullptr);

    QList<DeviceInfo> devices() const;
    void handleAnnounce(const DeviceInfo &device);
    void handleBye(const QString &deviceId);
    void removeExpired(const QDateTime &now, int timeoutSeconds = 10);

signals:
    void devicesChanged(QList<DeviceInfo> devices);

private:
    void emitChanged();

    QString m_localDeviceId;
    QHash<QString, DeviceInfo> m_devices;
};
