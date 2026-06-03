#include "network/DiscoveryService.h"

#include "protocol/MessageTypes.h"
#include "protocol/ProtocolCodec.h"

#include <QHostAddress>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUdpSocket>

DiscoveryService::DiscoveryService(QObject *parent)
    : QObject(parent)
{
}

DiscoveryService::~DiscoveryService()
{
    stop();
}

bool DiscoveryService::start(const DeviceInfo &localDevice, quint16 udpPort)
{
    stop();

    m_localDevice = localDevice;
    m_udpPort = udpPort;

    m_socket = new QUdpSocket(this);
    const bool bound = m_socket->bind(QHostAddress::AnyIPv4, m_udpPort,
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!bound) {
        emit errorOccurred(QStringLiteral("无法绑定 UDP 发现端口：%1").arg(m_socket->errorString()));
        m_socket->deleteLater();
        m_socket = nullptr;
        return false;
    }

    connect(m_socket, &QUdpSocket::readyRead, this, &DiscoveryService::readPendingDatagrams);

    m_timer = new QTimer(this);
    m_timer->setInterval(3000);
    connect(m_timer, &QTimer::timeout, this, &DiscoveryService::sendAnnounce);
    m_timer->start();

    sendAnnounce();
    return true;
}

void DiscoveryService::stop()
{
    sendBye();

    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }

    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void DiscoveryService::sendAnnounce()
{
    if (!m_socket) {
        return;
    }

    const QJsonObject json = ProtocolCodec::encodeAnnounce(m_localDevice, QDateTime::currentSecsSinceEpoch());
    const QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    const qint64 written = m_socket->writeDatagram(payload, QHostAddress::Broadcast, m_udpPort);
    if (written < 0) {
        emit errorOccurred(QStringLiteral("发送设备公告失败：%1").arg(m_socket->errorString()));
    }
}

void DiscoveryService::sendBye()
{
    if (!m_socket || m_localDevice.deviceId.isEmpty()) {
        return;
    }

    QJsonObject json;
    json["type"] = MessageTypes::Bye;
    json["version"] = 1;
    json["deviceId"] = m_localDevice.deviceId;

    const QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    m_socket->writeDatagram(payload, QHostAddress::Broadcast, m_udpPort);
}

void DiscoveryService::readPendingDatagrams()
{
    if (!m_socket) {
        return;
    }

    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_socket->pendingDatagramSize()));

        QHostAddress sender;
        quint16 senderPort = 0;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        Q_UNUSED(senderPort);

        const QJsonDocument document = QJsonDocument::fromJson(datagram);
        if (!document.isObject()) {
            emit errorOccurred(QStringLiteral("收到非法 UDP 发现消息"));
            continue;
        }

        const QJsonObject json = document.object();
        const QString type = json.value("type").toString();
        if (type == MessageTypes::Announce) {
            const AnnounceResult result = ProtocolCodec::decodeAnnounce(json, sender.toString());
            if (!result.isValid) {
                emit errorOccurred(result.error);
                continue;
            }

            if (result.device.deviceId != m_localDevice.deviceId) {
                emit deviceAnnounced(result.device);
            }
        } else if (type == MessageTypes::Bye) {
            const ByeResult result = ProtocolCodec::decodeBye(json);
            if (!result.isValid) {
                emit errorOccurred(result.error);
                continue;
            }

            if (result.deviceId != m_localDevice.deviceId) {
                emit deviceLeft(result.deviceId);
            }
        }
    }
}
