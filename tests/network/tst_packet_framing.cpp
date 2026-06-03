#include <QtTest>
#include "network/PacketReader.h"
#include "network/PacketWriter.h"
#include "protocol/ProtocolTypes.h"

class PacketFramingTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesSingleControlFrame()
    {
        QJsonObject body;
        body["type"] = "send_accept";
        body["version"] = 1;
        body["transferId"] = "transfer-1";

        PacketReader reader;
        reader.append(PacketWriter::encodeControlFrame(body));

        const QList<PacketFrame> frames = reader.takeFrames();

        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames.first().header.value("type").toString(), QStringLiteral("send_accept"));
        QCOMPARE(frames.first().hasPayload, false);
    }

    void parsesCombinedControlFrames()
    {
        QJsonObject first;
        first["type"] = "send_accept";
        first["version"] = 1;
        first["transferId"] = "transfer-1";

        QJsonObject second;
        second["type"] = "transfer_done";
        second["version"] = 1;
        second["transferId"] = "transfer-1";

        PacketReader reader;
        reader.append(PacketWriter::encodeControlFrame(first) + PacketWriter::encodeControlFrame(second));

        const QList<PacketFrame> frames = reader.takeFrames();

        QCOMPARE(frames.size(), 2);
        QCOMPARE(frames.at(0).header.value("type").toString(), QStringLiteral("send_accept"));
        QCOMPARE(frames.at(1).header.value("type").toString(), QStringLiteral("transfer_done"));
    }

    void waitsForPartialFrame()
    {
        QJsonObject body;
        body["type"] = "send_accept";
        body["version"] = 1;
        body["transferId"] = "transfer-1";

        const QByteArray frame = PacketWriter::encodeControlFrame(body);

        PacketReader reader;
        reader.append(frame.left(3));
        QVERIFY(reader.takeFrames().isEmpty());

        reader.append(frame.mid(3));
        const QList<PacketFrame> frames = reader.takeFrames();

        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames.first().header.value("transferId").toString(), QStringLiteral("transfer-1"));
    }

    void rejectsOversizedJsonFrame()
    {
        QByteArray bytes;
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << quint32(kMaxJsonFrameSize + 1);

        PacketReader reader;
        reader.append(bytes);

        QVERIFY(reader.takeFrames().isEmpty());
        QVERIFY(!reader.errorString().isEmpty());
    }

    void treatsFileDoneSizeAsControlField()
    {
        QJsonObject body;
        body["type"] = "file_done";
        body["version"] = 1;
        body["transferId"] = "transfer-1";
        body["fileId"] = "file-1";
        body["size"] = 12;

        PacketReader reader;
        reader.append(PacketWriter::encodeControlFrame(body));

        const QList<PacketFrame> frames = reader.takeFrames();

        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames.first().header.value("type").toString(), QStringLiteral("file_done"));
        QCOMPARE(frames.first().hasPayload, false);
    }
};

QTEST_MAIN(PacketFramingTest)
#include "tst_packet_framing.moc"
