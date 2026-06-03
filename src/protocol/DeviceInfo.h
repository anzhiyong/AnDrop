#pragma once

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

struct DeviceInfo
{
    QString deviceId;
    QString deviceName;
    QString platform;
    QString ipAddress;
    quint16 tcpPort = 53317;
    QDateTime lastSeen;
};

Q_DECLARE_METATYPE(DeviceInfo)
Q_DECLARE_METATYPE(QList<DeviceInfo>)
