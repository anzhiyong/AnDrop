#include "storage/ConfigStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

#include <utility>

namespace {

quint16 validPortOrDefault(int value, quint16 fallback)
{
    if (value < 1 || value > 65535) {
        return fallback;
    }

    return static_cast<quint16>(value);
}

QString defaultDownloadDir()
{
    const QString downloadRoot = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadRoot.isEmpty()) {
        return QDir::home().filePath(QStringLiteral("Downloads/AnDrop"));
    }

    return QDir(downloadRoot).filePath(QStringLiteral("AnDrop"));
}

}

ConfigStore::ConfigStore(QString configPath)
    : m_configPath(std::move(configPath))
{
}

AppConfig ConfigStore::load()
{
    QFile file(m_configPath);
    if (!file.exists()) {
        AppConfig config = defaults();
        save(config);
        return config;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        AppConfig config = defaults();
        save(config);
        return config;
    }

    const QByteArray bytes = file.readAll();
    file.close();

    const QJsonDocument document = QJsonDocument::fromJson(bytes);
    if (!document.isObject()) {
        AppConfig config = defaults();
        save(config);
        return config;
    }

    const QJsonObject json = document.object();
    AppConfig config;
    config.deviceId = json.value("deviceId").toString();
    config.deviceName = json.value("deviceName").toString(QStringLiteral("AnDrop"));
    config.downloadDir = json.value("downloadDir").toString(defaultDownloadDir());
    config.udpPort = validPortOrDefault(json.value("udpPort").toInt(53316), 53316);
    config.tcpPort = validPortOrDefault(json.value("tcpPort").toInt(53317), 53317);
    config.receiveMode = json.value("receiveMode").toString(QStringLiteral("AskEveryTime"));

    config = normalize(config);
    save(config);
    return config;
}

bool ConfigStore::save(const AppConfig &config, QString *errorMessage) const
{
    const AppConfig normalized = normalize(config);

    QDir dir = QFileInfo(m_configPath).dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法创建配置目录");
        }
        return false;
    }

    QJsonObject json;
    json["deviceId"] = normalized.deviceId;
    json["deviceName"] = normalized.deviceName;
    json["downloadDir"] = normalized.downloadDir;
    json["udpPort"] = static_cast<int>(normalized.udpPort);
    json["tcpPort"] = static_cast<int>(normalized.tcpPort);
    json["receiveMode"] = normalized.receiveMode;

    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法写入配置文件");
        }
        return false;
    }

    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    return true;
}

AppConfig ConfigStore::defaults() const
{
    AppConfig config;
    config.deviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    config.downloadDir = defaultDownloadDir();
    return config;
}

AppConfig ConfigStore::normalize(const AppConfig &config) const
{
    AppConfig normalized = config;

    if (normalized.deviceId.isEmpty()) {
        normalized.deviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    if (normalized.deviceName.trimmed().isEmpty()) {
        normalized.deviceName = QStringLiteral("AnDrop");
    }

    if (normalized.downloadDir.trimmed().isEmpty()) {
        normalized.downloadDir = defaultDownloadDir();
    }

    normalized.udpPort = validPortOrDefault(normalized.udpPort, 53316);
    normalized.tcpPort = validPortOrDefault(normalized.tcpPort, 53317);

    if (normalized.receiveMode.trimmed().isEmpty()) {
        normalized.receiveMode = QStringLiteral("AskEveryTime");
    }

    return normalized;
}
