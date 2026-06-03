#include <QtTest>
#include "core/DeviceManager.h"

class DeviceManagerTest : public QObject
{
    Q_OBJECT

private:
    DeviceInfo makeDevice(const QString &id, const QString &name, const QDateTime &lastSeen) const
    {
        DeviceInfo device;
        device.deviceId = id;
        device.deviceName = name;
        device.platform = "macos";
        device.ipAddress = "127.0.0.1";
        device.tcpPort = 53317;
        device.lastSeen = lastSeen;
        return device;
    }

private slots:
    void ignoresLocalDevice()
    {
        DeviceManager manager("local-id");
        manager.handleAnnounce(makeDevice("local-id", QStringLiteral("本机"), QDateTime::currentDateTimeUtc()));

        QVERIFY(manager.devices().isEmpty());
    }

    void addsAndRefreshesDeviceWithoutDuplicate()
    {
        DeviceManager manager("local-id");
        const QDateTime now = QDateTime::currentDateTimeUtc();

        manager.handleAnnounce(makeDevice("remote-1", QStringLiteral("旧名称"), now));
        manager.handleAnnounce(makeDevice("remote-1", QStringLiteral("新名称"), now.addSecs(1)));

        const QList<DeviceInfo> devices = manager.devices();
        QCOMPARE(devices.size(), 1);
        QCOMPARE(devices.first().deviceName, QStringLiteral("新名称"));
        QCOMPARE(devices.first().lastSeen, now.addSecs(1));
    }

    void removesExpiredDevices()
    {
        DeviceManager manager("local-id");
        const QDateTime now = QDateTime::currentDateTimeUtc();

        manager.handleAnnounce(makeDevice("remote-1", QStringLiteral("远端"), now.addSecs(-11)));
        manager.removeExpired(now, 10);

        QVERIFY(manager.devices().isEmpty());
    }

    void removesDeviceOnBye()
    {
        DeviceManager manager("local-id");

        manager.handleAnnounce(makeDevice("remote-1", QStringLiteral("远端"), QDateTime::currentDateTimeUtc()));
        manager.handleBye("remote-1");

        QVERIFY(manager.devices().isEmpty());
    }
};

QTEST_MAIN(DeviceManagerTest)
#include "tst_device_manager.moc"
