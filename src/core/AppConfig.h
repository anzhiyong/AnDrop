#pragma once

#include <QString>

struct AppConfig
{
    QString deviceId;
    QString deviceName = QStringLiteral("AnDrop");
    QString downloadDir;
    quint16 udpPort = 53316;
    quint16 tcpPort = 53317;
    QString receiveMode = QStringLiteral("AskEveryTime");
};
