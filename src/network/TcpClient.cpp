#include "network/TcpClient.h"

#include "network/PacketReader.h"
#include "network/PacketWriter.h"
#include "protocol/MessageTypes.h"
#include "protocol/ProtocolTypes.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpSocket>
#include <QUuid>

namespace {

struct LocalFile
{
    QString id;
    QString path;
    QString name;
    qint64 size = 0;
};

QJsonObject makeControlMessage(const QString &type, const QString &transferId)
{
    QJsonObject json;
    json["type"] = type;
    json["version"] = kProtocolVersion;
    json["transferId"] = transferId;
    return json;
}

}

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
{
}

void TcpClient::setLocalDeviceId(QString deviceId)
{
    m_localDeviceId = std::move(deviceId);
}

void TcpClient::sendFiles(DeviceInfo target, QStringList filePaths)
{
    QList<LocalFile> files;
    qint64 totalBytes = 0;

    for (const QString &path : filePaths) {
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile()) {
            emit transferFailed(QString(), QStringLiteral("发送文件不存在：%1").arg(path));
            return;
        }

        LocalFile file;
        file.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        file.path = info.absoluteFilePath();
        file.name = info.fileName();
        file.size = info.size();
        files.append(file);
        totalBytes += file.size;
    }

    if (files.isEmpty()) {
        emit transferFailed(QString(), QStringLiteral("没有可发送的文件"));
        return;
    }

    const QString transferId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_cancelledTransferId.clear();

    QTcpSocket socket;
    socket.connectToHost(target.ipAddress, target.tcpPort);
    if (!socket.waitForConnected(5000)) {
        emit transferFailed(transferId, QStringLiteral("连接目标设备失败：%1").arg(socket.errorString()));
        return;
    }

    QJsonArray fileArray;
    for (const LocalFile &file : files) {
        QJsonObject item;
        item["id"] = file.id;
        item["name"] = file.name;
        item["size"] = file.size;
        item["sha256"] = QJsonValue::Null;
        item["relativePath"] = file.name;
        fileArray.append(item);
    }

    QJsonObject request;
    request["type"] = MessageTypes::SendRequest;
    request["version"] = kProtocolVersion;
    request["transferId"] = transferId;
    request["fromDeviceId"] = m_localDeviceId.isEmpty() ? QStringLiteral("unknown-device") : m_localDeviceId;
    request["files"] = fileArray;
    request["totalSize"] = totalBytes;

    socket.write(PacketWriter::encodeControlFrame(request));
    if (!socket.waitForBytesWritten(5000)) {
        emit transferFailed(transferId, QStringLiteral("发送传输请求失败：%1").arg(socket.errorString()));
        return;
    }

    // 第一版先使用阻塞等待逻辑，调用方会把 TcpClient 放到网络线程中，避免阻塞 UI。
    if (!socket.waitForReadyRead(30000)) {
        emit transferFailed(transferId, QStringLiteral("等待接收方确认超时"));
        return;
    }

    PacketReader reader;
    reader.append(socket.readAll());
    const QList<PacketFrame> frames = reader.takeFrames();
    if (frames.isEmpty() || frames.first().header.value("type").toString() != MessageTypes::SendAccept) {
        emit transferFailed(transferId, QStringLiteral("接收方未接受传输"));
        return;
    }

    qint64 sentBytes = 0;
    for (const LocalFile &fileInfo : files) {
        QFile file(fileInfo.path);
        if (!file.open(QIODevice::ReadOnly)) {
            emit transferFailed(transferId, QStringLiteral("无法读取发送文件：%1").arg(fileInfo.path));
            return;
        }

        qint64 offset = 0;
        while (!file.atEnd()) {
            if (m_cancelledTransferId == transferId) {
                socket.write(PacketWriter::encodeControlFrame(makeControlMessage(MessageTypes::TransferCancel, transferId)));
                socket.waitForBytesWritten(1000);
                emit transferFailed(transferId, QStringLiteral("传输已取消"));
                return;
            }

            const QByteArray payload = file.read(kDefaultChunkSize);
            QJsonObject header;
            header["type"] = MessageTypes::FileChunk;
            header["version"] = kProtocolVersion;
            header["transferId"] = transferId;
            header["fileId"] = fileInfo.id;
            header["offset"] = offset;
            header["size"] = payload.size();

            socket.write(PacketWriter::encodeBinaryFrame(header, payload));
            if (!socket.waitForBytesWritten(5000)) {
                emit transferFailed(transferId, QStringLiteral("发送文件数据失败：%1").arg(socket.errorString()));
                return;
            }

            offset += payload.size();
            sentBytes += payload.size();
            emit transferProgress(transferId, sentBytes, totalBytes);
        }

        QJsonObject done = makeControlMessage(MessageTypes::FileDone, transferId);
        done["fileId"] = fileInfo.id;
        done["size"] = fileInfo.size;
        socket.write(PacketWriter::encodeControlFrame(done));
        socket.waitForBytesWritten(5000);
    }

    socket.write(PacketWriter::encodeControlFrame(makeControlMessage(MessageTypes::TransferDone, transferId)));
    socket.waitForBytesWritten(5000);
    emit transferCompleted(transferId);
}

void TcpClient::cancel(QString transferId)
{
    m_cancelledTransferId = std::move(transferId);
}
