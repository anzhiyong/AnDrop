#include "protocol/ProtocolCodec.h"

#include "protocol/MessageTypes.h"

#include <QJsonArray>
#include <QJsonValue>

namespace {

bool hasString(const QJsonObject &json, const QString &key)
{
    return json.contains(key) && json.value(key).isString() && !json.value(key).toString().isEmpty();
}

}

QJsonObject ProtocolCodec::encodeAnnounce(const DeviceInfo &device, qint64 timestamp)
{
    QJsonArray features;
    features.append(QStringLiteral("file"));
    features.append(QStringLiteral("folder"));

    QJsonObject json;
    json["type"] = MessageTypes::Announce;
    json["version"] = kProtocolVersion;
    json["deviceId"] = device.deviceId;
    json["deviceName"] = device.deviceName;
    json["platform"] = device.platform;
    json["tcpPort"] = static_cast<int>(device.tcpPort);
    json["features"] = features;
    json["timestamp"] = timestamp;
    return json;
}

DecodeResult ProtocolCodec::validateVersion(const QJsonObject &json)
{
    DecodeResult result;

    if (!json.contains("version") || !json.value("version").isDouble()) {
        result.error = QStringLiteral("协议消息缺少版本号");
        return result;
    }

    if (json.value("version").toInt() != kProtocolVersion) {
        result.error = QStringLiteral("协议版本不支持");
        return result;
    }

    result.isValid = true;
    return result;
}

AnnounceResult ProtocolCodec::decodeAnnounce(const QJsonObject &json, const QString &ipAddress)
{
    AnnounceResult result;

    const DecodeResult version = validateVersion(json);
    if (!version.isValid) {
        result.error = version.error;
        return result;
    }

    if (json.value("type").toString() != MessageTypes::Announce) {
        result.error = QStringLiteral("消息类型不是设备公告");
        return result;
    }

    if (!hasString(json, "deviceId") || !hasString(json, "deviceName") ||
        !hasString(json, "platform") || !json.value("tcpPort").isDouble()) {
        result.error = QStringLiteral("设备公告字段不完整");
        return result;
    }

    const int tcpPort = json.value("tcpPort").toInt();
    if (tcpPort < 1 || tcpPort > 65535) {
        result.error = QStringLiteral("设备公告端口非法");
        return result;
    }

    result.device.deviceId = json.value("deviceId").toString();
    result.device.deviceName = json.value("deviceName").toString();
    result.device.platform = json.value("platform").toString();
    result.device.ipAddress = ipAddress;
    result.device.tcpPort = static_cast<quint16>(tcpPort);
    result.device.lastSeen = QDateTime::currentDateTimeUtc();
    result.isValid = true;
    return result;
}

ByeResult ProtocolCodec::decodeBye(const QJsonObject &json)
{
    ByeResult result;

    const DecodeResult version = validateVersion(json);
    if (!version.isValid) {
        result.error = version.error;
        return result;
    }

    if (json.value("type").toString() != MessageTypes::Bye) {
        result.error = QStringLiteral("消息类型不是设备离线");
        return result;
    }

    if (!hasString(json, "deviceId")) {
        result.error = QStringLiteral("设备离线消息缺少 deviceId");
        return result;
    }

    result.deviceId = json.value("deviceId").toString();
    result.isValid = true;
    return result;
}

SendRequest ProtocolCodec::decodeSendRequest(const QJsonObject &json)
{
    SendRequest request;

    const DecodeResult version = validateVersion(json);
    if (!version.isValid) {
        request.error = version.error;
        return request;
    }

    if (json.value("type").toString() != MessageTypes::SendRequest) {
        request.error = QStringLiteral("消息类型不是发送请求");
        return request;
    }

    if (!hasString(json, "transferId")) {
        request.error = QStringLiteral("发送请求缺少 transferId");
        return request;
    }

    if (!hasString(json, "fromDeviceId")) {
        request.error = QStringLiteral("发送请求缺少 fromDeviceId");
        return request;
    }

    if (!json.contains("files") || !json.value("files").isArray()) {
        request.error = QStringLiteral("发送请求缺少文件列表");
        return request;
    }

    if (!json.contains("totalSize") || !json.value("totalSize").isDouble()) {
        request.error = QStringLiteral("发送请求缺少总大小");
        return request;
    }

    request.transferId = json.value("transferId").toString();
    request.fromDeviceId = json.value("fromDeviceId").toString();
    request.totalSize = static_cast<qint64>(json.value("totalSize").toDouble());

    const QJsonArray files = json.value("files").toArray();
    if (files.isEmpty()) {
        request.error = QStringLiteral("发送请求文件列表为空");
        return request;
    }

    for (const QJsonValue &value : files) {
        if (!value.isObject()) {
            request.error = QStringLiteral("文件清单格式非法");
            return request;
        }

        const QJsonObject fileJson = value.toObject();
        if (!hasString(fileJson, "id") || !hasString(fileJson, "name") ||
            !hasString(fileJson, "relativePath") ||
            !fileJson.contains("size") || !fileJson.value("size").isDouble()) {
            request.error = QStringLiteral("文件清单字段不完整");
            return request;
        }

        FileManifest file;
        file.id = fileJson.value("id").toString();
        file.name = fileJson.value("name").toString();
        file.size = static_cast<qint64>(fileJson.value("size").toDouble());
        file.relativePath = fileJson.value("relativePath").toString();

        if (fileJson.contains("sha256") && fileJson.value("sha256").isString()) {
            file.sha256 = fileJson.value("sha256").toString();
        }

        request.files.append(file);
    }

    request.isValid = true;
    return request;
}
