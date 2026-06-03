#pragma once

#include "protocol/DeviceInfo.h"

#include <QObject>

class QTimer;
class QUdpSocket;

class DiscoveryService : public QObject
{
    Q_OBJECT

public:
    explicit DiscoveryService(QObject *parent = nullptr);
    ~DiscoveryService() override;

public slots:
    bool start(const DeviceInfo &localDevice, quint16 udpPort);
    void stop();
    void sendAnnounce();
    void sendBye();

signals:
    void deviceAnnounced(DeviceInfo device);
    void deviceLeft(QString deviceId);
    void errorOccurred(QString message);

private slots:
    void readPendingDatagrams();

private:
    DeviceInfo m_localDevice;
    quint16 m_udpPort = 53316;
    QUdpSocket *m_socket = nullptr;
    QTimer *m_timer = nullptr;
};
