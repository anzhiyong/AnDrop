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
