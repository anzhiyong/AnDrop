#include "app/AppContext.h"

#include "core/DeviceManager.h"
#include "core/TransferManager.h"
#include "core/TransferTask.h"
#include "network/DiscoveryService.h"
#include "network/PacketReader.h"
#include "network/PacketWriter.h"
#include "network/TcpClient.h"
#include "network/TcpServer.h"
#include "protocol/MessageTypes.h"
#include "protocol/ProtocolCodec.h"
#include "storage/ReceivedFileStore.h"
#include "storage/ConfigStore.h"
#include "ui/ReceiveDialog.h"

#include <QApplication>
#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QHash>
#include <QJsonObject>
#include <QMetaObject>
#include <QMetaType>
#include <QTcpSocket>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThread>

namespace {

QString configFilePath()
{
    const QString overridePath = qEnvironmentVariable("ANDROP_CONFIG_PATH");
    if (!overridePath.isEmpty()) {
        return overridePath;
    }

    QString root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (root.isEmpty()) {
        root = QDir::home().filePath(QStringLiteral(".config/AnDrop"));
    }

    return QDir(root).filePath(QStringLiteral("config.json"));
}

quint16 portFromEnvironment(const char *name, quint16 fallback)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    if (!ok || value < 1 || value > 65535) {
        return fallback;
    }

    return static_cast<quint16>(value);
}

bool askUserToReceive(const SendRequest &request, const AppConfig &config)
{
    bool accepted = false;
    QMetaObject::invokeMethod(qApp, [&accepted, request, config]() {
        ReceiveDialog dialog;
        dialog.setSummary(request.fromDeviceId, request.files.size(), request.totalSize, config.downloadDir);
        accepted = dialog.exec() == QDialog::Accepted;
    }, Qt::BlockingQueuedConnection);
    return accepted;
}

void processIncomingSocket(QTcpSocket *socket, const AppConfig &config)
{
    if (!socket) {
        return;
    }

    socket->setParent(nullptr);
    PacketReader reader;

    if (!socket->waitForReadyRead(30000)) {
        socket->deleteLater();
        return;
    }

    reader.append(socket->readAll());
    QList<PacketFrame> frames = reader.takeFrames();
    if (frames.isEmpty()) {
        socket->deleteLater();
        return;
    }

    const SendRequest request = ProtocolCodec::decodeSendRequest(frames.first().header);
    if (!request.isValid) {
        QJsonObject error;
        error["type"] = MessageTypes::Error;
        error["version"] = kProtocolVersion;
        error["transferId"] = frames.first().header.value("transferId").toString();
        error["code"] = "protocol_error";
        error["message"] = request.error;
        socket->write(PacketWriter::encodeControlFrame(error));
        socket->waitForBytesWritten(1000);
        socket->deleteLater();
        return;
    }

    if (!askUserToReceive(request, config)) {
        QJsonObject reject;
        reject["type"] = MessageTypes::SendReject;
        reject["version"] = kProtocolVersion;
        reject["transferId"] = request.transferId;
        reject["reason"] = "user_rejected";
        socket->write(PacketWriter::encodeControlFrame(reject));
        socket->waitForBytesWritten(1000);
        socket->deleteLater();
        return;
    }

    QHash<QString, FileManifest> manifests;
    for (const FileManifest &file : request.files) {
        manifests.insert(file.id, file);
    }

    ReceivedFileStore store(config.downloadDir);
    QJsonObject accept;
    accept["type"] = MessageTypes::SendAccept;
    accept["version"] = kProtocolVersion;
    accept["transferId"] = request.transferId;
    socket->write(PacketWriter::encodeControlFrame(accept));
    socket->waitForBytesWritten(1000);

    while (socket->state() == QAbstractSocket::ConnectedState) {
        if (!socket->waitForReadyRead(30000)) {
            break;
        }

        reader.append(socket->readAll());
        frames = reader.takeFrames();
        for (const PacketFrame &frame : frames) {
            const QString type = frame.header.value("type").toString();
            if (type == MessageTypes::FileChunk) {
                const QString fileId = frame.header.value("fileId").toString();
                if (!manifests.contains(fileId)) {
                    continue;
                }

                QString error;
                const QString tempPath = store.temporaryPath(request.transferId, manifests.value(fileId).relativePath, &error);
                if (tempPath.isEmpty()) {
                    continue;
                }

                QFile output(tempPath);
                if (!output.open(QIODevice::ReadWrite)) {
                    continue;
                }
                output.seek(static_cast<qint64>(frame.header.value("offset").toDouble()));
                output.write(frame.payload);
            } else if (type == MessageTypes::FileDone) {
                const QString fileId = frame.header.value("fileId").toString();
                if (!manifests.contains(fileId)) {
                    continue;
                }

                QString error;
                const QString tempPath = store.temporaryPath(request.transferId, manifests.value(fileId).relativePath, &error);
                if (!tempPath.isEmpty()) {
                    store.completeFile(tempPath, manifests.value(fileId).relativePath, &error);
                }
            } else if (type == MessageTypes::TransferDone || type == MessageTypes::TransferCancel) {
                socket->disconnectFromHost();
                socket->deleteLater();
                return;
            }
        }
    }

    socket->deleteLater();
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
    m_config.udpPort = portFromEnvironment("ANDROP_UDP_PORT", m_config.udpPort);
    m_config.tcpPort = portFromEnvironment("ANDROP_TCP_PORT", m_config.tcpPort);

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
    connect(m_tcpServer, &TcpServer::incomingSocket, m_tcpServer, [this](QTcpSocket *socket) {
        processIncomingSocket(socket, m_config);
    });

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
