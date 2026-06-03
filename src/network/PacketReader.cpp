#include "network/PacketReader.h"

#include "protocol/MessageTypes.h"
#include "protocol/ProtocolTypes.h"

#include <QDataStream>
#include <QJsonDocument>

#include <limits>

void PacketReader::append(const QByteArray &bytes)
{
    if (!m_error.isEmpty()) {
        return;
    }

    m_buffer.append(bytes);
}

QList<PacketFrame> PacketReader::takeFrames()
{
    QList<PacketFrame> frames;

    while (m_error.isEmpty()) {
        if (m_buffer.size() < static_cast<int>(sizeof(quint32))) {
            break;
        }

        QDataStream lengthStream(m_buffer.left(sizeof(quint32)));
        lengthStream.setByteOrder(QDataStream::BigEndian);

        quint32 headerLength = 0;
        lengthStream >> headerLength;

        if (headerLength > static_cast<quint32>(kMaxJsonFrameSize)) {
            m_error = QStringLiteral("JSON 帧超过最大限制");
            m_buffer.clear();
            break;
        }

        const int prefixSize = static_cast<int>(sizeof(quint32));
        if (m_buffer.size() < prefixSize + static_cast<int>(headerLength)) {
            break;
        }

        const QByteArray jsonBytes = m_buffer.mid(prefixSize, static_cast<int>(headerLength));
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(jsonBytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            m_error = QStringLiteral("JSON 帧格式非法");
            m_buffer.clear();
            break;
        }

        PacketFrame frame;
        frame.header = document.object();

        qint64 payloadSize = 0;
        if (frame.header.value("type").toString() == MessageTypes::FileChunk &&
            frame.header.contains("size") && frame.header.value("size").isDouble()) {
            payloadSize = static_cast<qint64>(frame.header.value("size").toDouble());
            if (payloadSize < 0) {
                m_error = QStringLiteral("二进制载荷长度非法");
                m_buffer.clear();
                break;
            }
        }

        const int headerConsumed = prefixSize + static_cast<int>(headerLength);
        if (payloadSize > 0) {
            if (payloadSize > std::numeric_limits<int>::max()) {
                m_error = QStringLiteral("二进制载荷长度超过当前实现限制");
                m_buffer.clear();
                break;
            }

            if (m_buffer.size() < headerConsumed + static_cast<int>(payloadSize)) {
                break;
            }

            frame.payload = m_buffer.mid(headerConsumed, static_cast<int>(payloadSize));
            frame.hasPayload = true;
        }

        const int consumed = headerConsumed + static_cast<int>(payloadSize);
        m_buffer.remove(0, consumed);
        frames.append(frame);
    }

    return frames;
}

QString PacketReader::errorString() const
{
    return m_error;
}
