#pragma once

#include "core/AppConfig.h"
#include "protocol/DeviceInfo.h"

#include <QObject>
#include <QStringList>

class ConfigStore;
class DeviceManager;
class DiscoveryService;
class TcpClient;
class TcpServer;
class TransferManager;
class QThread;

class AppContext : public QObject
{
    Q_OBJECT

public:
    explicit AppContext(QObject *parent = nullptr);
    ~AppContext() override;

    bool start(QString *errorMessage = nullptr);

    DeviceManager *deviceManager() const;
    TransferManager *transferManager() const;
    AppConfig config() const;

public slots:
    void sendFilesToDevice(DeviceInfo device, QStringList filePaths);

private:
    AppConfig m_config;
    ConfigStore *m_configStore = nullptr;
    DeviceManager *m_deviceManager = nullptr;
    TransferManager *m_transferManager = nullptr;
    DiscoveryService *m_discoveryService = nullptr;
    TcpServer *m_tcpServer = nullptr;
    TcpClient *m_tcpClient = nullptr;
    QThread *m_networkThread = nullptr;
};
