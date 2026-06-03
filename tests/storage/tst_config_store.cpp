#include <QtTest>
#include "storage/ConfigStore.h"

class ConfigStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void createsDefaultsWhenFileMissing()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ConfigStore store(dir.filePath("config.json"));
        const AppConfig config = store.load();

        QVERIFY(!config.deviceId.isEmpty());
        QCOMPARE(config.deviceName, QStringLiteral("AnDrop"));
        QCOMPARE(config.udpPort, quint16(53316));
        QCOMPARE(config.tcpPort, quint16(53317));
        QCOMPARE(config.receiveMode, QStringLiteral("AskEveryTime"));
        QVERIFY(config.downloadDir.endsWith(QStringLiteral("AnDrop")));
    }

    void keepsGeneratedDeviceId()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ConfigStore store(dir.filePath("config.json"));
        const AppConfig first = store.load();
        const AppConfig second = store.load();

        QCOMPARE(second.deviceId, first.deviceId);
    }

    void repairsInvalidPorts()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile file(dir.filePath("config.json"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({
            "deviceId": "device-1",
            "deviceName": "AnDrop",
            "downloadDir": "/tmp/AnDrop",
            "udpPort": 70000,
            "tcpPort": 0,
            "receiveMode": "AskEveryTime"
        })");
        file.close();

        ConfigStore store(dir.filePath("config.json"));
        const AppConfig config = store.load();

        QCOMPARE(config.udpPort, quint16(53316));
        QCOMPARE(config.tcpPort, quint16(53317));
    }

    void savesChineseDeviceName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ConfigStore store(dir.filePath("config.json"));
        AppConfig config = store.load();
        config.deviceName = QStringLiteral("中文设备");

        QString error;
        QVERIFY2(store.save(config, &error), qPrintable(error));

        const AppConfig reloaded = store.load();
        QCOMPARE(reloaded.deviceName, QStringLiteral("中文设备"));
        QCOMPARE(reloaded.deviceId, config.deviceId);
    }
};

QTEST_MAIN(ConfigStoreTest)
#include "tst_config_store.moc"
