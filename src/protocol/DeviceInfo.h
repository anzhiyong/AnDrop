#pragma once

#include <QDateTime>
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
