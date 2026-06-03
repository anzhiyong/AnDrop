#include "app/AppContext.h"

#include "core/DeviceManager.h"
#include "core/TransferManager.h"
#include "core/TransferTask.h"
#include "network/DiscoveryService.h"
#include "network/TcpClient.h"
#include "network/TcpServer.h"
#include "storage/ConfigStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QMetaType>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThread>

namespace {

QString configFilePath()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (root.isEmpty()) {
        root = QDir::home().filePath(QStringLiteral(".config/AnDrop"));
    }

    return QDir(root).filePath(QStringLiteral("config.json"));
}

}

AppContext::AppContext(QObject *parent)
    : QObject(parent)
{
}

AppContext::~AppContext()
{
    if (m_discoveryService) {
        QMetaObject::invokeMethod(m_discoveryService, "stop", Qt::BlockingQueuedConnection);
    }
    if (m_tcpServer) {
        QMetaObject::invokeMethod(m_tcpServer, "stop", Qt::BlockingQueuedConnection);
    }

    if (m_networkThread) {
        m_networkThread->quit();
        m_networkThread->wait();
    }

    delete m_configStore;
    m_configStore = nullptr;
}

bool AppContext::start(QString *errorMessage)
{
    qRegisterMetaType<DeviceInfo>("DeviceInfo");
    qRegisterMetaType<QList<DeviceInfo>>("QList<DeviceInfo>");
    qRegisterMetaType<TransferTask>("TransferTask");
    qRegisterMetaType<QList<TransferTask>>("QList<TransferTask>");

    m_configStore = new ConfigStore(configFilePath());
    m_config = m_configStore->load();

    m_deviceManager = new DeviceManager(m_config.deviceId, this);
    m_transferManager = new TransferManager(this);

    m_networkThread = new QThread(this);
    m_discoveryService = new DiscoveryService();
    m_tcpServer = new TcpServer();
    m_tcpClient = new TcpClient();
    m_tcpClient->setLocalDeviceId(m_config.deviceId);

    m_discoveryService->moveToThread(m_networkThread);
    m_tcpServer->moveToThread(m_networkThread);
    m_tcpClient->moveToThread(m_networkThread);

    connect(m_networkThread, &QThread::finished, m_discoveryService, &QObject::deleteLater);
    connect(m_networkThread, &QThread::finished, m_tcpServer, &QObject::deleteLater);
    connect(m_networkThread, &QThread::finished, m_tcpClient, &QObject::deleteLater);

    connect(m_discoveryService, &DiscoveryService::deviceAnnounced, m_deviceManager, &DeviceManager::handleAnnounce);
    connect(m_discoveryService, &DiscoveryService::deviceLeft, m_deviceManager, &DeviceManager::handleBye);

    connect(m_tcpClient, &TcpClient::transferProgress, m_transferManager, &TransferManager::updateProgress);
    connect(m_tcpClient, &TcpClient::transferCompleted, m_transferManager, &TransferManager::markCompleted);
    connect(m_tcpClient, &TcpClient::transferFailed, m_transferManager, &TransferManager::markFailed);

    m_networkThread->start();

    bool tcpStarted = false;
    QMetaObject::invokeMethod(m_tcpServer, [this, &tcpStarted]() {
        tcpStarted = m_tcpServer->start(m_config.tcpPort);
    }, Qt::BlockingQueuedConnection);
    if (!tcpStarted) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("TCP 服务启动失败");
        }
        return false;
    }

    DeviceInfo localDevice;
    localDevice.deviceId = m_config.deviceId;
    localDevice.deviceName = m_config.deviceName;
    localDevice.platform = QSysInfo::prettyProductName();
    localDevice.tcpPort = m_config.tcpPort;

    bool discoveryStarted = false;
    QMetaObject::invokeMethod(m_discoveryService, [this, localDevice, &discoveryStarted]() {
        discoveryStarted = m_discoveryService->start(localDevice, m_config.udpPort);
    }, Qt::BlockingQueuedConnection);
    if (!discoveryStarted) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("UDP 发现服务启动失败");
        }
        return false;
    }

    return true;
}

DeviceManager *AppContext::deviceManager() const
{
    return m_deviceManager;
}

TransferManager *AppContext::transferManager() const
{
    return m_transferManager;
}

AppConfig AppContext::config() const
{
    return m_config;
}

void AppContext::sendFilesToDevice(DeviceInfo device, QStringList filePaths)
{
    qint64 totalBytes = 0;
    for (const QString &path : filePaths) {
        totalBytes += QFileInfo(path).size();
    }

    m_transferManager->createOutgoingTransfer(totalBytes);
    QMetaObject::invokeMethod(m_tcpClient, [this, device, filePaths]() {
        m_tcpClient->sendFiles(device, filePaths);
    }, Qt::QueuedConnection);
}
