# AnDrop MVP 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 按 `docs/superpowers/specs/2026-06-03-androp-design.md` 实现 AnDrop 第一版 MVP：Qt5 Widgets 应用、配置持久化、UDP 设备发现、TCP 文件传输、接收确认、进度展示和取消传输。

**Architecture:** 项目采用 UI、应用、核心、网络、协议、存储、平台分层。UI 不直接操作 socket，设备状态通过 `DeviceManager`，传输状态通过 `TransferManager`，网络对象运行在独立线程，文件 IO 通过 worker 或线程池执行。

**Tech Stack:** C++17、Qt5 Widgets、Qt5 Network、Qt5 Test、CMake、JSON 使用 Qt `QJsonDocument/QJsonObject`。

---

## 0. 全局实现规则

- 所有新增代码注释必须使用中文。
- 注释只解释设计意图、边界条件或复杂流程，不做逐行翻译。
- 每个任务完成后必须提交一次 Git commit。
- 每个实现任务先写测试，再写实现。
- 除 UI 任务外，核心逻辑必须优先放在可单元测试的类中。
- 不允许 UI 类直接读写 socket。
- 不允许把文件数据 base64 后塞进 JSON。
- 不允许静默覆盖已有文件。

## 1. 目标文件结构

```text
CMakeLists.txt
README.md

src/
  app/
    main.cpp
    AppContext.h
    AppContext.cpp
  core/
    AppConfig.h
    DeviceManager.h
    DeviceManager.cpp
    TransferManager.h
    TransferManager.cpp
    TransferTask.h
    TransferTask.cpp
  network/
    DiscoveryService.h
    DiscoveryService.cpp
    PacketReader.h
    PacketReader.cpp
    PacketWriter.h
    PacketWriter.cpp
    TcpClient.h
    TcpClient.cpp
    TcpServer.h
    TcpServer.cpp
  protocol/
    DeviceInfo.h
    FileManifest.h
    MessageTypes.h
    ProtocolCodec.h
    ProtocolCodec.cpp
    ProtocolTypes.h
  storage/
    ConfigStore.h
    ConfigStore.cpp
    ReceivedFileStore.h
    ReceivedFileStore.cpp
  ui/
    MainWindow.h
    MainWindow.cpp
    HomePage.h
    HomePage.cpp
    DeviceListPage.h
    DeviceListPage.cpp
    TransferPage.h
    TransferPage.cpp
    SettingsPage.h
    SettingsPage.cpp
    ReceiveDialog.h
    ReceiveDialog.cpp

tests/
  CMakeLists.txt
  protocol/
    tst_protocol_codec.cpp
  network/
    tst_packet_framing.cpp
  core/
    tst_device_manager.cpp
    tst_transfer_task.cpp
  storage/
    tst_config_store.cpp
    tst_received_file_store.cpp
```

## 2. 任务清单

### Task 1: 创建 Qt5/CMake 工程骨架

**Files:**
- Create: `CMakeLists.txt`
- Create: `README.md`
- Create: `src/app/main.cpp`
- Create: `tests/CMakeLists.txt`
- Modify: `.gitignore`

- [ ] **Step 1: 写最小 CMake 工程**

`CMakeLists.txt` 必须包含：

```cmake
cmake_minimum_required(VERSION 3.16)

project(AnDrop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network Test Concurrent)

add_executable(AnDrop
    src/app/main.cpp
)

target_link_libraries(AnDrop PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::Concurrent
)

enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: 写最小入口**

`src/app/main.cpp` 必须先保持可运行：

```cpp
#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLabel label(QStringLiteral("AnDrop"));
    label.resize(360, 180);
    label.show();

    return app.exec();
}
```

- [ ] **Step 3: 写测试入口 CMake**

`tests/CMakeLists.txt` 先创建空测试工程入口：

```cmake
add_custom_target(androp_tests)
```

- [ ] **Step 4: 更新忽略规则**

`.gitignore` 增加：

```gitignore
.cache/
Testing/
```

- [ ] **Step 5: 验证构建**

Run:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected:

```text
Build succeeds.
No tests were found!!!
```

- [ ] **Step 6: 提交**

```bash
git add CMakeLists.txt README.md src/app/main.cpp tests/CMakeLists.txt .gitignore
git commit -m "chore: add Qt5 CMake project skeleton"
```

### Task 2: 实现协议类型和 JSON 编解码

**Files:**
- Create: `src/protocol/MessageTypes.h`
- Create: `src/protocol/DeviceInfo.h`
- Create: `src/protocol/FileManifest.h`
- Create: `src/protocol/ProtocolTypes.h`
- Create: `src/protocol/ProtocolCodec.h`
- Create: `src/protocol/ProtocolCodec.cpp`
- Create: `tests/protocol/tst_protocol_codec.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: 写失败测试**

`tests/protocol/tst_protocol_codec.cpp` 覆盖：

```cpp
#include <QtTest>
#include "protocol/ProtocolCodec.h"

class ProtocolCodecTest : public QObject
{
    Q_OBJECT

private slots:
    void encodesAnnounceMessage()
    {
        DeviceInfo device;
        device.deviceId = "device-1";
        device.deviceName = QStringLiteral("测试设备");
        device.platform = "macos";
        device.ipAddress = "127.0.0.1";
        device.tcpPort = 53317;

        const QJsonObject json = ProtocolCodec::encodeAnnounce(device, 123);

        QCOMPARE(json.value("type").toString(), QStringLiteral("announce"));
        QCOMPARE(json.value("version").toInt(), 1);
        QCOMPARE(json.value("deviceId").toString(), QStringLiteral("device-1"));
        QCOMPARE(json.value("deviceName").toString(), QStringLiteral("测试设备"));
        QCOMPARE(json.value("tcpPort").toInt(), 53317);
    }

    void decodesSendRequest()
    {
        QJsonObject file;
        file["id"] = "file-1";
        file["name"] = "demo.txt";
        file["size"] = 12;
        file["sha256"] = QJsonValue::Null;
        file["relativePath"] = "demo.txt";

        QJsonArray files;
        files.append(file);

        QJsonObject json;
        json["type"] = "send_request";
        json["version"] = 1;
        json["transferId"] = "transfer-1";
        json["fromDeviceId"] = "device-1";
        json["files"] = files;
        json["totalSize"] = 12;

        const auto request = ProtocolCodec::decodeSendRequest(json);

        QVERIFY(request.isValid);
        QCOMPARE(request.transferId, QStringLiteral("transfer-1"));
        QCOMPARE(request.files.size(), 1);
        QCOMPARE(request.files.first().relativePath, QStringLiteral("demo.txt"));
        QCOMPARE(request.totalSize, qint64(12));
    }

    void rejectsUnsupportedVersion()
    {
        QJsonObject json;
        json["type"] = "announce";
        json["version"] = 99;

        QCOMPARE(ProtocolCodec::validateVersion(json).isValid, false);
    }
};

QTEST_MAIN(ProtocolCodecTest)
#include "tst_protocol_codec.moc"
```

- [ ] **Step 2: 运行测试确认失败**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R ProtocolCodecTest
```

Expected:

```text
Compilation fails because protocol/ProtocolCodec.h does not exist.
```

- [ ] **Step 3: 实现最小协议类型**

`src/protocol/MessageTypes.h`：

```cpp
#pragma once

#include <QString>

namespace MessageTypes {
const QString Announce = QStringLiteral("announce");
const QString Bye = QStringLiteral("bye");
const QString SendRequest = QStringLiteral("send_request");
const QString SendAccept = QStringLiteral("send_accept");
const QString SendReject = QStringLiteral("send_reject");
const QString FileChunk = QStringLiteral("file_chunk");
const QString FileDone = QStringLiteral("file_done");
const QString TransferDone = QStringLiteral("transfer_done");
const QString TransferCancel = QStringLiteral("transfer_cancel");
const QString Error = QStringLiteral("error");
}
```

`src/protocol/DeviceInfo.h`：

```cpp
#pragma once

#include <QDateTime>
#include <QString>

struct DeviceInfo
{
    QString deviceId;
    QString deviceName;
    QString platform;
    QString ipAddress;
    quint16 tcpPort = 53317;
    QDateTime lastSeen;
};
```

`src/protocol/FileManifest.h`：

```cpp
#pragma once

#include <QString>

struct FileManifest
{
    QString id;
    QString name;
    qint64 size = 0;
    QString sha256;
    QString relativePath;
};
```

`src/protocol/ProtocolTypes.h`：

```cpp
#pragma once

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
```

- [ ] **Step 4: 实现 `ProtocolCodec`**

实现内容必须包含：

```cpp
#pragma once

#include "protocol/DeviceInfo.h"
#include "protocol/ProtocolTypes.h"

#include <QJsonObject>

class ProtocolCodec
{
public:
    static QJsonObject encodeAnnounce(const DeviceInfo &device, qint64 timestamp);
    static DecodeResult validateVersion(const QJsonObject &json);
    static SendRequest decodeSendRequest(const QJsonObject &json);
};
```

`src/protocol/ProtocolCodec.cpp` 必须：

- 写入 `type/version/deviceId/deviceName/platform/tcpPort/features/timestamp`。
- 校验 `version == kProtocolVersion`。
- 校验 `send_request` 的 `transferId/fromDeviceId/files/totalSize`。
- 对缺失字段返回 `isValid=false` 和中文错误信息。

- [ ] **Step 5: 接入 CMake**

新增 `androp_core` 静态库，测试链接该库。

- [ ] **Step 6: 运行测试确认通过**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R ProtocolCodecTest
```

Expected:

```text
100% tests passed
```

- [ ] **Step 7: 提交**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/protocol tests/protocol
git commit -m "feat: add protocol json codec"
```

### Task 3: 实现 TCP 帧读写工具

**Files:**
- Create: `src/network/PacketReader.h`
- Create: `src/network/PacketReader.cpp`
- Create: `src/network/PacketWriter.h`
- Create: `src/network/PacketWriter.cpp`
- Create: `tests/network/tst_packet_framing.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: 写失败测试**

测试必须覆盖：

- 单个控制帧编码后能解析。
- 两个控制帧粘在一起能分别解析。
- 半个帧先到达时不能提前输出。
- 超过 `kMaxJsonFrameSize` 的帧被拒绝。

核心测试数据：

```cpp
QJsonObject first;
first["type"] = "send_accept";
first["version"] = 1;
first["transferId"] = "transfer-1";

QJsonObject second;
second["type"] = "transfer_done";
second["version"] = 1;
second["transferId"] = "transfer-1";
```

- [ ] **Step 2: 运行测试确认失败**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R PacketFramingTest
```

Expected:

```text
Compilation fails because PacketReader does not exist.
```

- [ ] **Step 3: 实现帧工具**

`PacketWriter` 提供：

```cpp
class PacketWriter
{
public:
    static QByteArray encodeControlFrame(const QJsonObject &body);
    static QByteArray encodeBinaryFrame(const QJsonObject &header, const QByteArray &payload);
};
```

`PacketReader` 提供：

```cpp
struct PacketFrame
{
    QJsonObject header;
    QByteArray payload;
    bool hasPayload = false;
};

class PacketReader
{
public:
    void append(const QByteArray &bytes);
    QList<PacketFrame> takeFrames();
    QString errorString() const;

private:
    QByteArray m_buffer;
    QString m_error;
};
```

实现要求：

- 使用网络字节序写入 `uint32`。
- JSON 使用 `QJsonDocument::Compact`。
- 读取长度不足时保留缓存。
- 长度超过 `kMaxJsonFrameSize` 时设置中文错误。

- [ ] **Step 4: 运行测试确认通过**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R PacketFramingTest
```

Expected:

```text
100% tests passed
```

- [ ] **Step 5: 提交**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/network/PacketReader.* src/network/PacketWriter.* tests/network
git commit -m "feat: add tcp packet framing helpers"
```

### Task 4: 实现配置模型和持久化

**Files:**
- Create: `src/core/AppConfig.h`
- Create: `src/storage/ConfigStore.h`
- Create: `src/storage/ConfigStore.cpp`
- Create: `tests/storage/tst_config_store.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: 写失败测试**

测试必须覆盖：

- 配置文件不存在时创建默认配置。
- `deviceId` 首次生成后再次加载保持不变。
- 非法端口恢复为默认端口。
- 保存中文设备名后能再次读出。

断言示例：

```cpp
QVERIFY(!config.deviceId.isEmpty());
QCOMPARE(config.deviceName, QStringLiteral("AnDrop"));
QCOMPARE(config.udpPort, quint16(53316));
QCOMPARE(config.tcpPort, quint16(53317));
QCOMPARE(config.receiveMode, QStringLiteral("AskEveryTime"));
```

- [ ] **Step 2: 运行测试确认失败**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R ConfigStoreTest
```

Expected:

```text
Compilation fails because ConfigStore does not exist.
```

- [ ] **Step 3: 实现配置**

`AppConfig` 字段：

```cpp
struct AppConfig
{
    QString deviceId;
    QString deviceName = QStringLiteral("AnDrop");
    QString downloadDir;
    quint16 udpPort = 53316;
    quint16 tcpPort = 53317;
    QString receiveMode = QStringLiteral("AskEveryTime");
};
```

`ConfigStore` 接口：

```cpp
class ConfigStore
{
public:
    explicit ConfigStore(QString configPath);

    AppConfig load();
    bool save(const AppConfig &config, QString *errorMessage = nullptr) const;

private:
    QString m_configPath;
};
```

实现要求：

- 使用 JSON 文件持久化。
- 缺失 `deviceId` 时用 `QUuid::createUuid()` 创建。
- 默认下载目录使用 `QStandardPaths::DownloadLocation + "/AnDrop"`。
- 端口不在 `1..65535` 时恢复默认值。
- 错误信息使用中文。

- [ ] **Step 4: 运行测试确认通过**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R ConfigStoreTest
```

Expected:

```text
100% tests passed
```

- [ ] **Step 5: 提交**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/core/AppConfig.h src/storage/ConfigStore.* tests/storage/tst_config_store.cpp
git commit -m "feat: add app config persistence"
```

### Task 5: 实现设备管理器

**Files:**
- Create: `src/core/DeviceManager.h`
- Create: `src/core/DeviceManager.cpp`
- Create: `tests/core/tst_device_manager.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: 写失败测试**

测试必须覆盖：

- 忽略本机 `deviceId`。
- 新设备被加入列表。
- 相同 `deviceId` 的设备刷新，不重复添加。
- 超过 10 秒未刷新后被移除或标记离线。
- 收到 `bye` 后移除设备。

- [ ] **Step 2: 运行测试确认失败**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R DeviceManagerTest
```

Expected:

```text
Compilation fails because DeviceManager does not exist.
```

- [ ] **Step 3: 实现设备管理器**

接口：

```cpp
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(QString localDeviceId, QObject *parent = nullptr);

    QList<DeviceInfo> devices() const;
    void handleAnnounce(const DeviceInfo &device);
    void handleBye(const QString &deviceId);
    void removeExpired(const QDateTime &now, int timeoutSeconds = 10);

signals:
    void devicesChanged(QList<DeviceInfo> devices);

private:
    QString m_localDeviceId;
    QHash<QString, DeviceInfo> m_devices;
};
```

实现要求：

- 用 `deviceId` 作为 key。
- 收到本机 ID 直接返回。
- 每次列表变化才发 `devicesChanged`。
- 中文注释说明为什么不能用 IP 做主身份。

- [ ] **Step 4: 运行测试确认通过**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R DeviceManagerTest
```

Expected:

```text
100% tests passed
```

- [ ] **Step 5: 提交**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/core/DeviceManager.* tests/core/tst_device_manager.cpp
git commit -m "feat: add device manager"
```

### Task 6: 实现传输任务状态机

**Files:**
- Create: `src/core/TransferTask.h`
- Create: `src/core/TransferTask.cpp`
- Create: `tests/core/tst_transfer_task.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: 写失败测试**

测试必须覆盖：

- `Pending -> WaitingForReceiver -> Accepted -> Transferring -> Completed`。
- `WaitingForReceiver -> Rejected`。
- `Transferring -> Cancelled`。
- 非法状态流转不改变当前状态。
- 进度根据已传字节和总字节计算。

- [ ] **Step 2: 运行测试确认失败**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R TransferTaskTest
```

Expected:

```text
Compilation fails because TransferTask does not exist.
```

- [ ] **Step 3: 实现状态机**

接口：

```cpp
enum class TransferState
{
    Pending,
    WaitingForReceiver,
    Accepted,
    Transferring,
    Completed,
    Rejected,
    Cancelled,
    Failed
};

class TransferTask
{
public:
    TransferTask(QString transferId, qint64 totalBytes);

    QString transferId() const;
    TransferState state() const;
    qint64 totalBytes() const;
    qint64 transferredBytes() const;
    int progressPercent() const;
    QString errorMessage() const;

    bool transitionTo(TransferState next);
    void setTransferredBytes(qint64 bytes);
    void fail(QString message);

private:
    bool canTransition(TransferState from, TransferState to) const;

    QString m_transferId;
    TransferState m_state = TransferState::Pending;
    qint64 m_totalBytes = 0;
    qint64 m_transferredBytes = 0;
    QString m_errorMessage;
};
```

实现要求：

- 只允许设计文档中列出的状态流转。
- `fail()` 设置中文错误时允许进入 `Failed`。
- `progressPercent()` 在总大小为 0 时返回 0。

- [ ] **Step 4: 运行测试确认通过**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R TransferTaskTest
```

Expected:

```text
100% tests passed
```

- [ ] **Step 5: 提交**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/core/TransferTask.* tests/core/tst_transfer_task.cpp
git commit -m "feat: add transfer task state machine"
```

### Task 7: 实现接收文件存储规则

**Files:**
- Create: `src/storage/ReceivedFileStore.h`
- Create: `src/storage/ReceivedFileStore.cpp`
- Create: `tests/storage/tst_received_file_store.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: 写失败测试**

测试必须覆盖：

- 临时文件路径位于 `<downloadDir>/.androp_tmp/<transferId>/`。
- `../evil.txt` 被拒绝。
- 已存在 `demo.txt` 时生成 `demo (1).txt`。
- 完成后 `.part` 文件重命名为最终文件。
- 取消时临时目录被清理。

- [ ] **Step 2: 运行测试确认失败**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R ReceivedFileStoreTest
```

Expected:

```text
Compilation fails because ReceivedFileStore does not exist.
```

- [ ] **Step 3: 实现存储规则**

接口：

```cpp
class ReceivedFileStore
{
public:
    explicit ReceivedFileStore(QString downloadDir);

    QString temporaryPath(const QString &transferId, const QString &relativePath, QString *errorMessage = nullptr) const;
    QString finalPathFor(const QString &relativePath, QString *errorMessage = nullptr) const;
    bool completeFile(const QString &temporaryPath, const QString &relativePath, QString *errorMessage = nullptr) const;
    bool cleanupTransfer(const QString &transferId, QString *errorMessage = nullptr) const;

private:
    bool isSafeRelativePath(const QString &relativePath) const;
    QString m_downloadDir;
};
```

实现要求：

- 使用 `QDir::cleanPath` 校验路径。
- 禁止绝对路径。
- 禁止 `..` 路径段。
- 创建目录失败时返回中文错误。
- 目标重名时使用 `name (1).ext` 格式。

- [ ] **Step 4: 运行测试确认通过**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure -R ReceivedFileStoreTest
```

Expected:

```text
100% tests passed
```

- [ ] **Step 5: 提交**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/storage/ReceivedFileStore.* tests/storage/tst_received_file_store.cpp
git commit -m "feat: add received file storage rules"
```

### Task 8: 实现 UDP 发现服务

**Files:**
- Create: `src/network/DiscoveryService.h`
- Create: `src/network/DiscoveryService.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 定义服务接口**

接口：

```cpp
class DiscoveryService : public QObject
{
    Q_OBJECT

public:
    explicit DiscoveryService(QObject *parent = nullptr);

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
```

- [ ] **Step 2: 实现行为**

实现要求：

- `start()` 绑定 UDP 端口并允许地址复用。
- 每 3 秒发送一次 `announce`。
- `stop()` 发送 `bye` 并释放 socket/timer。
- 收到 `announce` 后用 `ProtocolCodec` 解析并发 `deviceAnnounced`。
- 收到 `bye` 后发 `deviceLeft`。
- 忽略本机 `deviceId`。
- 错误信息使用中文。

- [ ] **Step 3: 手动双实例验证**

Run:

```bash
cmake --build build
```

Expected:

```text
Build succeeds.
```

本任务先不要求 UI 展示，后续由 `AppContext` 接线后进行双实例验证。

- [ ] **Step 4: 提交**

```bash
git add CMakeLists.txt src/network/DiscoveryService.*
git commit -m "feat: add udp discovery service"
```

### Task 9: 实现 TCP 服务端、客户端和传输会话

**Files:**
- Create: `src/network/TcpServer.h`
- Create: `src/network/TcpServer.cpp`
- Create: `src/network/TcpClient.h`
- Create: `src/network/TcpClient.cpp`
- Create: `src/core/TransferManager.h`
- Create: `src/core/TransferManager.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 定义 `TcpServer`**

接口：

```cpp
class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(QObject *parent = nullptr);

public slots:
    bool start(quint16 port);
    void stop();

signals:
    void incomingSocket(QTcpSocket *socket);
    void errorOccurred(QString message);

private:
    QTcpServer *m_server = nullptr;
};
```

- [ ] **Step 2: 定义 `TcpClient`**

接口：

```cpp
class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);

public slots:
    void sendFiles(DeviceInfo target, QStringList filePaths);
    void cancel(QString transferId);

signals:
    void transferProgress(QString transferId, qint64 sentBytes, qint64 totalBytes);
    void transferCompleted(QString transferId);
    void transferFailed(QString transferId, QString message);
};
```

- [ ] **Step 3: 定义 `TransferManager`**

接口：

```cpp
class TransferManager : public QObject
{
    Q_OBJECT

public:
    explicit TransferManager(QObject *parent = nullptr);

    QList<TransferTask> tasks() const;

public slots:
    QString createOutgoingTransfer(qint64 totalBytes);
    void updateProgress(QString transferId, qint64 transferredBytes);
    void markCompleted(QString transferId);
    void markRejected(QString transferId);
    void markCancelled(QString transferId);
    void markFailed(QString transferId, QString message);

signals:
    void tasksChanged(QList<TransferTask> tasks);

private:
    QHash<QString, TransferTask> m_tasks;
};
```

- [ ] **Step 4: 实现最小传输**

实现要求：

- 发送方连接目标 TCP 端口。
- 发送 `send_request`。
- 等待 `send_accept` 后发送文件块。
- 每块大小使用 `kDefaultChunkSize`。
- 每个文件完成后发送 `file_done`。
- 所有文件完成后发送 `transfer_done`。
- 任意 socket 错误转为中文错误消息。
- 接收方第一版先通过 `TransferManager` 发信号给 UI，由 `ReceiveDialog` 决定接受或拒绝。

- [ ] **Step 5: 构建验证**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected:

```text
All existing tests pass.
```

- [ ] **Step 6: 提交**

```bash
git add CMakeLists.txt src/network/TcpServer.* src/network/TcpClient.* src/core/TransferManager.*
git commit -m "feat: add tcp transfer services"
```

### Task 10: 实现 Qt Widgets 页面骨架

**Files:**
- Create: `src/ui/MainWindow.h`
- Create: `src/ui/MainWindow.cpp`
- Create: `src/ui/HomePage.h`
- Create: `src/ui/HomePage.cpp`
- Create: `src/ui/DeviceListPage.h`
- Create: `src/ui/DeviceListPage.cpp`
- Create: `src/ui/TransferPage.h`
- Create: `src/ui/TransferPage.cpp`
- Create: `src/ui/SettingsPage.h`
- Create: `src/ui/SettingsPage.cpp`
- Create: `src/ui/ReceiveDialog.h`
- Create: `src/ui/ReceiveDialog.cpp`
- Modify: `src/app/main.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 实现主窗口结构**

UI 必须包含：

- 左侧或顶部导航。
- 设备页。
- 传输页。
- 设置页。

`MainWindow` 接口：

```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
};
```

- [ ] **Step 2: 实现页面最小能力**

页面要求：

- `HomePage` 显示本机设备名、接收状态、在线设备数量。
- `DeviceListPage` 显示设备列表，并有“选择文件发送”按钮。
- `TransferPage` 显示任务列表、进度、状态、取消按钮。
- `SettingsPage` 显示设备名、下载目录、UDP 端口、TCP 端口、接收模式。
- `ReceiveDialog` 显示发送方、文件数量、总大小、保存目录、接受和拒绝按钮。

- [ ] **Step 3: 替换入口**

`main.cpp` 改为启动 `MainWindow`。

- [ ] **Step 4: 构建验证**

Run:

```bash
cmake --build build
```

Expected:

```text
Build succeeds.
```

- [ ] **Step 5: 手动 UI 检查**

Run:

```bash
./build/AnDrop
```

Expected:

```text
Window opens.
Navigation switches between Devices, Transfers, Settings.
No visible text overflow in default window size.
```

- [ ] **Step 6: 提交**

```bash
git add CMakeLists.txt src/app/main.cpp src/ui
git commit -m "feat: add Qt widgets application shell"
```

### Task 11: 实现 AppContext 并完成服务接线

**Files:**
- Create: `src/app/AppContext.h`
- Create: `src/app/AppContext.cpp`
- Modify: `src/app/main.cpp`
- Modify: `src/ui/*.h`
- Modify: `src/ui/*.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: 定义 `AppContext`**

接口：

```cpp
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

private:
    AppConfig m_config;
    ConfigStore *m_configStore = nullptr;
    DeviceManager *m_deviceManager = nullptr;
    TransferManager *m_transferManager = nullptr;
    DiscoveryService *m_discoveryService = nullptr;
    TcpServer *m_tcpServer = nullptr;
    QThread *m_networkThread = nullptr;
};
```

- [ ] **Step 2: 实现启动流程**

实现要求：

- 加载配置。
- 创建 `DeviceManager` 和 `TransferManager`。
- 创建网络线程。
- 将 `DiscoveryService` 和 `TcpServer` 移动到网络线程。
- 启动 TCP 服务端。
- 启动 UDP 发现服务。
- 将发现事件连接到 `DeviceManager`。
- 错误通过中文消息返回。

- [ ] **Step 3: UI 连接服务**

实现要求：

- `DeviceListPage` 订阅设备列表更新。
- `TransferPage` 订阅任务列表更新。
- 设置页展示当前配置。
- 发送按钮调用 `TransferManager` 创建任务，并交给 `TcpClient` 发送。
- 收到接收请求时弹出 `ReceiveDialog`。

- [ ] **Step 4: 构建和测试**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected:

```text
All tests pass.
```

- [ ] **Step 5: 提交**

```bash
git add CMakeLists.txt src/app src/ui
git commit -m "feat: wire application services"
```

### Task 12: 双实例端到端验证和修复

**Files:**
- Create: `docs/manual-test-mvp.md`
- Modify: `src/network/TcpClient.cpp`
- Modify: `src/network/TcpServer.cpp`
- Modify: `src/network/DiscoveryService.cpp`
- Modify: `src/core/TransferManager.cpp`
- Modify: `src/storage/ReceivedFileStore.cpp`
- Modify: `src/ui/DeviceListPage.cpp`
- Modify: `src/ui/TransferPage.cpp`
- Modify: `src/ui/ReceiveDialog.cpp`
- Modify: `tests/core/tst_transfer_task.cpp`
- Modify: `tests/storage/tst_received_file_store.cpp`

- [ ] **Step 1: 写手动测试文档**

`docs/manual-test-mvp.md` 必须记录：

```markdown
# AnDrop MVP 手动测试

## 单机双实例

1. 启动实例 A，使用 UDP 端口 53316，TCP 端口 53317。
2. 启动实例 B，使用 UDP 端口 53318，TCP 端口 53319。
3. 确认两边设备列表能看到对方。
4. 从 A 选择一个小文件发送给 B。
5. 在 B 上点击接受。
6. 确认文件保存到 B 的下载目录。
7. 确认 A 和 B 的传输页都显示完成。
8. 再次发送同名文件，确认自动重命名。
9. 发送大文件并取消，确认任务显示取消且临时文件被清理。
10. 发送文件并拒绝，确认发送方显示已拒绝。
```

- [ ] **Step 2: 运行自动测试**

Run:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected:

```text
All tests pass.
```

- [ ] **Step 3: 运行单机双实例测试**

Run:

```bash
./build/AnDrop
```

再启动第二个实例，使用设置页修改端口后重启网络服务或重启应用。

Expected:

```text
两个实例互相发现。
文件能从一个实例发送到另一个实例。
接收方确认弹窗正常。
进度显示正常。
取消、拒绝、同名重命名正常。
```

- [ ] **Step 4: 修复验证中发现的问题**

修复规则：

- 如果是协议问题，优先补充协议测试。
- 如果是路径问题，优先补充 `ReceivedFileStore` 测试。
- 如果是状态问题，优先补充 `TransferTask` 或 `TransferManager` 测试。
- 如果是 UI 展示问题，修复页面代码并手动复测。

- [ ] **Step 5: 提交**

```bash
git add src docs/manual-test-mvp.md tests
git commit -m "test: document and verify mvp manual flow"
```

## 3. 最终验收命令

MVP 完成前必须执行：

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

并完成 `docs/manual-test-mvp.md` 中的单机双实例手动测试。

## 4. 计划自查

- 设计文档中的模块划分已映射到 `src/app`、`src/ui`、`src/core`、`src/network`、`src/protocol`、`src/storage`。
- UDP 发现由 Task 8 覆盖。
- TCP 文件传输由 Task 3、Task 9 覆盖。
- 配置持久化由 Task 4 覆盖。
- 接收文件存储规则由 Task 7 覆盖。
- 传输状态机由 Task 6 覆盖。
- 页面结构由 Task 10 覆盖。
- 服务接线和线程模型由 Task 11 覆盖。
- MVP 验收由 Task 12 覆盖。
- 代码中文注释规范已列入全局实现规则。
