#pragma once

#include <QByteArray>
#include <QJsonObject>

class PacketWriter
{
public:
    static QByteArray encodeControlFrame(const QJsonObject &body);
    static QByteArray encodeBinaryFrame(const QJsonObject &header, const QByteArray &payload);
};
