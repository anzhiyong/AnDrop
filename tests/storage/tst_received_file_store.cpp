#include <QtTest>
#include "storage/ReceivedFileStore.h"

class ReceivedFileStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void createsTemporaryPathInsideTransferDirectory()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ReceivedFileStore store(dir.path());
        QString error;
        const QString path = store.temporaryPath("transfer-1", "nested/demo.txt", &error);

        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY(path.contains(QStringLiteral(".androp_tmp/transfer-1/nested/demo.txt.part")));
    }

    void rejectsUnsafeRelativePath()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ReceivedFileStore store(dir.path());
        QString error;
        const QString path = store.temporaryPath("transfer-1", "../evil.txt", &error);

        QVERIFY(path.isEmpty());
        QVERIFY(!error.isEmpty());
    }

    void autoRenamesExistingFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile existing(QDir(dir.path()).filePath("demo.txt"));
        QVERIFY(existing.open(QIODevice::WriteOnly));
        existing.write("old");
        existing.close();

        ReceivedFileStore store(dir.path());
        QString error;
        const QString finalPath = store.finalPathFor("demo.txt", &error);

        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(QFileInfo(finalPath).fileName(), QStringLiteral("demo (1).txt"));
    }

    void completesPartFileToFinalPath()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ReceivedFileStore store(dir.path());
        QString error;
        const QString tempPath = store.temporaryPath("transfer-1", "demo.txt", &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        QFile temp(tempPath);
        QVERIFY(temp.open(QIODevice::WriteOnly));
        temp.write("hello");
        temp.close();

        QVERIFY2(store.completeFile(tempPath, "demo.txt", &error), qPrintable(error));

        const QString finalPath = QDir(dir.path()).filePath("demo.txt");
        QVERIFY(QFileInfo::exists(finalPath));
        QVERIFY(!QFileInfo::exists(tempPath));
    }

    void cleansTemporaryTransferDirectory()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        ReceivedFileStore store(dir.path());
        QString error;
        const QString tempPath = store.temporaryPath("transfer-1", "demo.txt", &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        QFile temp(tempPath);
        QVERIFY(temp.open(QIODevice::WriteOnly));
        temp.write("hello");
        temp.close();

        QVERIFY2(store.cleanupTransfer("transfer-1", &error), qPrintable(error));
        QVERIFY(!QFileInfo::exists(QDir(dir.path()).filePath(".androp_tmp/transfer-1")));
    }
};

QTEST_MAIN(ReceivedFileStoreTest)
#include "tst_received_file_store.moc"
