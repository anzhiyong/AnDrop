#pragma once

#include "protocol/DeviceInfo.h"
#include "protocol/FileManifest.h"

#include <QList>
#include <QString>

constexpr int kProtocolVersion = 1;
constexpr int kDefaultUdpPort = 53316;
constexpr int kDefaultTcpPort = 53317;
constexpr int kDefaultChunkSize = 256 * 1024;
constexpr int kMaxJsonFrameSize = 1024 * 1024;

struct DecodeResult
{
    bool isValid = false;
    QString error;
};

struct SendRequest
{
    bool isValid = false;
    QString error;
    QString transferId;
    QString fromDeviceId;
    QList<FileManifest> files;
    qint64 totalSize = 0;
};

struct AnnounceResult
{
    bool isValid = false;
    QString error;
    DeviceInfo device;
};

struct ByeResult
{
    bool isValid = false;
    QString error;
    QString deviceId;
};
