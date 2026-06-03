#pragma once

#include "protocol/DeviceInfo.h"
#include "protocol/ProtocolTypes.h"

#include <QJsonObject>

class ProtocolCodec
{
public:
    static QJsonObject encodeAnnounce(const DeviceInfo &device, qint64 timestamp);
    static DecodeResult validateVersion(const QJsonObject &json);
    static AnnounceResult decodeAnnounce(const QJsonObject &json, const QString &ipAddress);
    static ByeResult decodeBye(const QJsonObject &json);
    static SendRequest decodeSendRequest(const QJsonObject &json);
};
