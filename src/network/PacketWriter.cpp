#include "network/PacketWriter.h"

#include <QDataStream>
#include <QJsonDocument>

namespace {

QByteArray encodeLengthPrefixedJson(const QJsonObject &json)
{
    const QByteArray jsonBytes = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << static_cast<quint32>(jsonBytes.size());
    frame.append(jsonBytes);
    return frame;
}

}

QByteArray PacketWriter::encodeControlFrame(const QJsonObject &body)
{
    return encodeLengthPrefixedJson(body);
}

QByteArray PacketWriter::encodeBinaryFrame(const QJsonObject &header, const QByteArray &payload)
{
    QByteArray frame = encodeLengthPrefixedJson(header);
    frame.append(payload);
    return frame;
}
