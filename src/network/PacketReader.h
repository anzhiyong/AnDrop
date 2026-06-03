#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>

struct PacketFrame
{
    QJsonObject header;
    QByteArray payload;
    bool hasPayload = false;
};

class PacketReader
{
public:
    void append(const QByteArray &bytes);
    QList<PacketFrame> takeFrames();
    QString errorString() const;

private:
    QByteArray m_buffer;
    QString m_error;
};
