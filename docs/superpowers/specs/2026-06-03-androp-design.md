# AnDrop Qt5/C++ 局域网文件传输工具设计文档

日期：2026-06-03
状态：后续实现必须严格遵循的基线设计

## 1. 产品目标

AnDrop 是一个使用 Qt5 + C++ 开发的桌面端局域网文件传输工具，定位类似 LocalSend / AirDrop。

第一版目标是先做稳定可用的局域网文件传输能力：

- 发现同一局域网内的其他设备。
- 向选中的设备发送一个或多个文件。
- 接收方在接收前必须确认接受或拒绝。
- 展示传输进度、速度、状态和错误信息。
- 将接收到的文件保存到用户配置的下载目录。

第一版不追求完整 AirDrop 级别能力。设备配对、TLS 加密、断点续传、可信设备和移动端兼容都属于后续扩展，但架构上必须预留扩展空间。

## 2. 第一版非目标

第一版不实现以下能力：

- 互联网中继传输。
- 账号登录或云同步。
- NAT 穿透。
- 移动端客户端。
- 端到端加密。
- 二维码配对。
- 断点续传。
- 文件预览。
- 完整传输历史搜索。

这些能力只能在后续单独更新设计文档后再实现。

## 3. 整体架构

AnDrop 必须采用分层架构。

```text
UI 层
  MainWindow、页面、弹窗、控件
        |
应用层
  设备编排、传输编排、设置管理
        |
核心层
  发现状态、传输状态机、协议对象
        |
网络层
  UDP 发现、TCP 服务端/客户端、帧读写
        |
存储 / 平台层
  配置文件、接收文件、系统通知、平台集成
```

规则：

- UI 类不能直接持有或操作 socket。
- 网络类不能直接更新 UI 控件。
- 传输状态必须通过 `TransferManager` 流转。
- 设备状态必须通过 `DeviceManager` 流转。
- 跨线程通信必须使用 Qt signal/slot。
- 协议消息必须先表示为明确的 C++ 结构，再进行序列化。

## 4. 模块划分

项目应采用以下模块结构。除非后续设计文档明确变更，否则实现必须按照此结构推进。

```text
app/
  main.cpp
  AppContext

ui/
  MainWindow
  HomePage
  DeviceListPage
  TransferPage
  SettingsPage
  ReceiveDialog
  TransferItemWidget

core/
  DeviceManager
  TransferManager
  TransferTask
  TransferSession
  AppConfig

network/
  DiscoveryService
  TcpServer
  TcpClient
  ProtocolCodec
  PacketReader
  PacketWriter

protocol/
  ProtocolTypes
  MessageTypes
  DeviceInfo
  FileManifest

storage/
  ConfigStore
  ReceivedFileStore
  HistoryStore

platform/
  SystemTray
  NotificationService
  FileOpener
```

### 4.1 `AppContext`

`AppContext` 持有应用生命周期内长期存在的服务：

- `AppConfig`
- `DeviceManager`
- `TransferManager`
- `DiscoveryService`
- `TcpServer`
- 平台服务

它负责在应用启动时完成各服务之间的 signal/slot 连接。

### 4.2 `DeviceManager`

职责：

- 管理本机设备身份。
- 保存当前在线的远端设备。
- 按 `deviceId` 去重。
- 收到发现消息时刷新设备最后在线时间。
- 超时后将设备标记为离线。
- 向 UI 发出设备列表更新信号。

`DeviceManager` 不能直接执行 UDP socket 操作。它只接收 `DiscoveryService` 发出的发现事件。

### 4.3 `TransferManager`

职责：

- 创建发送任务。
- 创建接收任务。
- 持有传输任务列表。
- 维护传输状态流转。
- 聚合进度、速度和错误状态。
- 向 UI 发出任务更新信号。
- 协调网络服务和文件 IO 服务。

所有传输生命周期变化都必须经过 `TransferManager`。

### 4.4 `DiscoveryService`

职责：

- 广播或组播本机设备公告。
- 监听远端设备公告。
- 发出解析后的发现事件。
- 应用关闭时尽量发送 `bye` 消息。

`DiscoveryService` 必须运行在 UI 线程之外。

### 4.5 TCP 组件

`TcpServer` 负责接受传入的传输连接。

`TcpClient` 负责创建传出的传输连接。

`PacketReader` 和 `PacketWriter` 负责 TCP 帧读写，屏蔽粘包和拆包细节。

`ProtocolCodec` 负责协议对象和 JSON 之间的转换。

## 5. 网络协议

协议分为两部分：

- UDP 设备发现协议。
- TCP 文件传输协议。

### 5.1 协议版本

每一条协议消息都必须包含：

- `version`
- `type`

第一版协议版本固定为 `1`。

如果收到不支持的协议版本，接收方必须优雅拒绝，并给出明确错误。

### 5.2 设备身份

每个 AnDrop 安装实例必须生成并持久化一个稳定的 `deviceId`。

推荐格式：

```text
UUID 字符串
```

设备身份字段：

```json
{
  "deviceId": "uuid",
  "deviceName": "MacBook",
  "platform": "macos"
}
```

`deviceId` 是主身份。IP 地址不能作为主身份，因为 IP 可能变化。

### 5.3 UDP 设备发现

默认 UDP 发现端口：

```text
53316
```

应用应支持在设置中修改该端口。

设备公告消息：

```json
{
  "type": "announce",
  "version": 1,
  "deviceId": "uuid",
  "deviceName": "MacBook",
  "platform": "macos",
  "tcpPort": 53317,
  "features": ["file", "folder"],
  "timestamp": 1780470000
}
```

设备离线消息：

```json
{
  "type": "bye",
  "version": 1,
  "deviceId": "uuid"
}
```

发现行为：

- 应用启动时发送 `announce`。
- 应用运行期间周期性发送 `announce`。
- 默认公告间隔：3 秒。
- 默认离线超时：10 秒。
- 忽略来自本机 `deviceId` 的发现消息。
- 已存在设备应刷新状态，不能重复添加。

### 5.4 TCP 传输帧格式

TCP 消息必须使用显式长度帧。

控制帧：

```text
uint32 bodyLength
json body
```

文件数据帧：

```text
uint32 headerLength
json header
binary payload
```

规则：

- 多字节整数必须使用网络字节序。
- JSON 必须使用 UTF-8。
- 文件载荷必须是原始二进制。
- 文件载荷不能使用 base64。
- `PacketReader` 必须处理不完整读取。
- `PacketWriter` 必须处理不完整写入。

### 5.5 传输消息类型

必须支持的消息类型：

```text
send_request
send_accept
send_reject
file_chunk
file_done
transfer_done
transfer_cancel
error
```

`send_request`：

```json
{
  "type": "send_request",
  "version": 1,
  "transferId": "uuid",
  "fromDeviceId": "uuid",
  "files": [
    {
      "id": "file-1",
      "name": "demo.zip",
      "size": 10485760,
      "sha256": null,
      "relativePath": "demo.zip"
    }
  ],
  "totalSize": 10485760
}
```

`send_accept`：

```json
{
  "type": "send_accept",
  "version": 1,
  "transferId": "uuid"
}
```

`send_reject`：

```json
{
  "type": "send_reject",
  "version": 1,
  "transferId": "uuid",
  "reason": "user_rejected"
}
```

`file_chunk` 头部：

```json
{
  "type": "file_chunk",
  "version": 1,
  "transferId": "uuid",
  "fileId": "file-1",
  "offset": 0,
  "size": 262144
}
```

`file_done`：

```json
{
  "type": "file_done",
  "version": 1,
  "transferId": "uuid",
  "fileId": "file-1",
  "size": 10485760
}
```

`transfer_done`：

```json
{
  "type": "transfer_done",
  "version": 1,
  "transferId": "uuid"
}
```

`transfer_cancel`：

```json
{
  "type": "transfer_cancel",
  "version": 1,
  "transferId": "uuid",
  "reason": "user_cancelled"
}
```

`error`：

```json
{
  "type": "error",
  "version": 1,
  "transferId": "uuid",
  "code": "protocol_error",
  "message": "Unsupported message type"
}
```

### 5.6 文件块大小

默认文件块大小：

```text
256 KiB
```

后续可以将其改为可配置项，但第一版使用固定值。

## 6. 线程模型

应用必须使用以下线程模型：

```text
主线程 / UI 线程
  QWidget UI 和用户交互

网络线程
  UDP 发现
  TCP 服务端
  TCP 客户端会话
  协议帧读写

文件 IO 线程池
  文件扫描
  文件读取
  文件写入
  hash 计算
```

规则：

- UI 线程不能执行阻塞 socket IO。
- UI 线程不能执行大文件读写。
- 网络对象应作为 `QObject` 移动到专用 `QThread`。
- 文件工作应使用 `QThreadPool`、`QtConcurrent` 或专用 worker 对象。
- UI 更新必须通过 queued signal/slot 投递。
- 共享状态必须由单一 manager 持有，并通过不可变快照或值对象暴露。

## 7. 传输状态模型

每个传输任务都必须使用清晰的状态机。

必需状态：

```text
Pending
WaitingForReceiver
Accepted
Transferring
Completed
Rejected
Cancelled
Failed
```

允许的状态流转：

```text
Pending -> WaitingForReceiver
WaitingForReceiver -> Accepted
WaitingForReceiver -> Rejected
WaitingForReceiver -> Failed
Accepted -> Transferring
Transferring -> Completed
Transferring -> Cancelled
Transferring -> Failed
Pending -> Cancelled
WaitingForReceiver -> Cancelled
Accepted -> Cancelled
```

非法状态流转必须被忽略或记录日志，不能破坏 UI 状态。

## 8. 文件传输流程

### 8.1 启动与发现流程

```text
1. 应用启动。
2. AppConfig 加载或创建本机 deviceId。
3. TcpServer 在配置的 TCP 端口开始监听。
4. DiscoveryService 在配置的 UDP 端口开始监听。
5. DiscoveryService 发送 announce。
6. DiscoveryService 周期性发送 announce。
7. DeviceManager 接收远端 announce 事件。
8. DeviceManager 更新在线设备列表。
9. UI 刷新设备列表。
```

### 8.2 发送文件流程

```text
1. 用户选择目标设备。
2. 用户选择文件或文件夹。
3. TransferManager 创建 transferId。
4. TransferManager 创建发送 TransferTask。
5. TcpClient 连接目标设备 TCP 端口。
6. 发送方发送 send_request。
7. 接收方显示 ReceiveDialog。
8. 接收方发送 send_accept 或 send_reject。
9. 如果接受，发送方开始流式发送 file_chunk 帧。
10. 每个文件发送完成后发送 file_done。
11. 所有文件发送完成后发送 transfer_done。
12. 双方将任务标记为 Completed。
```

### 8.3 接收文件流程

```text
1. TcpServer 接受传入连接。
2. PacketReader 解析 send_request。
3. TransferManager 创建接收 TransferTask。
4. UI 显示 ReceiveDialog。
5. 如果用户接受，接收方创建临时输出文件。
6. 接收方按 file_chunk 的 offset 写入数据。
7. 接收方验证最终文件大小。
8. 接收方将临时文件重命名为最终文件。
9. 接收方将任务标记为 Completed。
```

### 8.4 取消流程

```text
1. 任意一方都可以取消非终态传输任务。
2. 取消方在可行时发送 transfer_cancel。
3. 本地停止文件 IO。
4. 本地关闭连接。
5. 接收方删除未完成的临时文件。
6. UI 将任务标记为 Cancelled。
```

### 8.5 失败处理

必须处理以下错误：

- 连接失败：任务标记为 `Failed`。
- 接收方拒绝：发送方任务标记为 `Rejected`。
- 协议版本不支持：发送 `error`，然后任务失败。
- 非法帧：发送 `error`，然后关闭连接。
- 磁盘空间不足：接收任务失败，并删除临时文件。
- 文件名冲突：自动重命名目标文件。
- 发送过程中源文件被删除：任务失败。
- 远端断开连接：当前任务失败。

## 9. 页面结构

第一版使用 Qt Widgets。

```text
MainWindow
  HomePage
  DeviceListPage
  TransferPage
  SettingsPage
  ReceiveDialog
```

### 9.1 `MainWindow`

`MainWindow` 包含主导航和页面容器。

必需导航项：

- 设备
- 传输
- 设置

### 9.2 `HomePage`

第一版的首页可以保持轻量。

必需展示信息：

- 本机设备名。
- 本机接收状态。
- 在线设备数量。
- 进入设备列表的快捷入口。

### 9.3 `DeviceListPage`

必需行为：

- 展示在线设备。
- 展示设备名、平台、IP 地址和最后在线状态。
- 允许选择一个设备。
- 允许选择文件并发送到该设备。

### 9.4 `TransferPage`

必需行为：

- 展示发送中和接收中的传输任务。
- 展示文件数量、总大小、进度百分比、速度和当前状态。
- 对活跃传输提供取消操作。
- 当前会话内保留已完成和失败任务。

### 9.5 `ReceiveDialog`

必需行为：

- 展示发送方设备名。
- 展示文件数量。
- 展示总大小。
- 展示保存目录。
- 提供接受和拒绝操作。

### 9.6 `SettingsPage`

必需设置项：

- 设备名。
- 下载目录。
- UDP 发现端口。
- TCP 传输端口。
- 接收模式。

接收模式：

```text
AskEveryTime
AutoAcceptTrusted
RejectAll
```

第一版只要求 `AskEveryTime` 实际可用。其他值可以先存在于配置模型中，作为后续扩展预留。

## 10. 配置管理

配置必须持久化到本地。

必需字段：

```json
{
  "deviceId": "uuid",
  "deviceName": "AnDrop",
  "downloadDir": "/Users/name/Downloads/AnDrop",
  "udpPort": 53316,
  "tcpPort": 53317,
  "receiveMode": "AskEveryTime"
}
```

规则：

- `deviceId` 必须只生成一次，之后持久化复用。
- 如果配置缺失或非法，应用必须重新创建安全默认值。
- 修改网络端口后，可以要求重启网络服务。

## 11. 文件存储规则

接收文件必须先写入临时文件。

推荐临时文件命名：

```text
<downloadDir>/.androp_tmp/<transferId>/<relativePath>.part
```

文件完成并通过大小校验后，再重命名到最终位置。

如果最终文件已存在，必须自动重命名：

```text
demo.zip
demo (1).zip
demo (2).zip
```

传输取消或失败时，必须删除未完成的临时文件。

## 12. 安全基线

第一版面向可信局域网使用。

必需的基础保护：

- 接收文件前必须询问用户。
- 接受前必须展示发送方名称和文件摘要。
- 拒绝不支持的协议版本。
- 限制最大 JSON 帧大小。
- 校验文件路径，防止目录穿越。
- 接收的相对路径不能逃逸下载目录。

暂缓实现的安全能力：

- TLS。
- 设备配对。
- 可信设备列表。
- 设备指纹校验。
- 一次性接收码。

## 13. 后续扩展方案

### 阶段 1：MVP

- Qt Widgets 应用外壳。
- 本地配置。
- UDP 设备发现。
- TCP 文件传输。
- 发送与接收确认。
- 传输进度。
- 取消活跃传输。

### 阶段 2：桌面体验增强

- 拖拽发送。
- 系统托盘。
- 原生通知。
- 传输历史。
- 更好的设备图标。
- 文件夹传输体验优化。

### 阶段 3：可靠性增强

- SHA-256 校验。
- 失败重试。
- 断点续传。
- 传输队列。
- 带宽限制。
- 中断临时文件恢复。

### 阶段 4：安全增强

- 可信设备列表。
- 配对流程。
- TLS 加密。
- 设备指纹展示。
- 可选的一次性接收码。

### 阶段 5：兼容性增强

- mDNS 发现。
- IPv6 支持。
- Windows 防火墙提示。
- 协议兼容性测试。
- 未来 Android / iOS 客户端。

## 14. 实现约束

后续实现必须遵守以下约束：

- 不能写直接读写 socket 的 UI 代码。
- 不能把文件传输逻辑放进 widget。
- 不能把文件载荷放进 JSON 或 base64。
- 不能在 UI 线程阻塞网络 IO 或文件 IO。
- 不能使用 IP 地址作为设备身份。
- 不能把未完成文件直接保存为最终文件。
- 不能静默覆盖已有文件。
- 不能在 MVP 端到端可用前加入高级功能。
- 代码中需要注释时，注释必须使用中文。
- 注释应解释设计意图、边界条件或复杂流程，不能写没有信息量的逐行翻译。
- Git 提交信息必须使用中文。

## 15. 测试策略

必须覆盖的测试和手动检查：

- 协议序列化和反序列化测试。
- TCP 帧拆包、粘包测试。
- `DeviceManager` 的新增、刷新、去重和超时测试。
- `TransferTask` 状态流转测试。
- 文件路径安全校验测试。
- 单机双实例测试，使用不同端口模拟两台设备。
- 两台机器局域网传输测试。
- 取消传输测试。
- 接收方拒绝测试。

## 16. MVP 验收标准

只有满足以下条件，MVP 才算完成：

- 两个 AnDrop 实例可以在同一局域网内发现彼此。
- 用户可以选择远端设备并发送至少一个文件。
- 接收方在接收前可以看到确认弹窗。
- 接收方可以接受或拒绝传输。
- 接受后的文件能完整到达，且大小正确。
- UI 在传输过程中展示进度。
- 任意一方可以取消活跃传输。
- 失败传输显示明确失败状态。
- 大文件传输期间 UI 仍保持响应。

## 17. 已固定的第一版决策

以下决策在第一版中固定：

- UI 框架：Qt Widgets。
- 构建目标：Qt5 + C++。
- 设备发现：UDP 公告消息。
- 文件传输：TCP socket 流。
- 控制消息编码：JSON。
- 文件载荷编码：原始二进制。
- 默认 UDP 端口：53316。
- 默认 TCP 端口：53317。
- 默认文件块大小：256 KiB。
- 第一版接收模式：每次询问。

任何对这些决策的修改，都必须先更新本文档或创建替代设计文档，再修改实现。
