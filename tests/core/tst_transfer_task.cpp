#include <QtTest>
#include "core/TransferTask.h"

class TransferTaskTest : public QObject
{
    Q_OBJECT

private slots:
    void completesHappyPath()
    {
        TransferTask task("transfer-1", 100);

        QVERIFY(task.transitionTo(TransferState::WaitingForReceiver));
        QVERIFY(task.transitionTo(TransferState::Accepted));
        QVERIFY(task.transitionTo(TransferState::Transferring));
        task.setTransferredBytes(100);
        QVERIFY(task.transitionTo(TransferState::Completed));

        QCOMPARE(task.state(), TransferState::Completed);
        QCOMPARE(task.progressPercent(), 100);
    }

    void rejectsWhileWaiting()
    {
        TransferTask task("transfer-1", 100);

        QVERIFY(task.transitionTo(TransferState::WaitingForReceiver));
        QVERIFY(task.transitionTo(TransferState::Rejected));

        QCOMPARE(task.state(), TransferState::Rejected);
    }

    void cancelsWhileTransferring()
    {
        TransferTask task("transfer-1", 100);

        QVERIFY(task.transitionTo(TransferState::WaitingForReceiver));
        QVERIFY(task.transitionTo(TransferState::Accepted));
        QVERIFY(task.transitionTo(TransferState::Transferring));
        QVERIFY(task.transitionTo(TransferState::Cancelled));

        QCOMPARE(task.state(), TransferState::Cancelled);
    }

    void ignoresInvalidTransition()
    {
        TransferTask task("transfer-1", 100);

        QVERIFY(!task.transitionTo(TransferState::Completed));
        QCOMPARE(task.state(), TransferState::Pending);
    }

    void calculatesProgress()
    {
        TransferTask task("transfer-1", 200);

        task.setTransferredBytes(50);
        QCOMPARE(task.progressPercent(), 25);

        task.setTransferredBytes(250);
        QCOMPARE(task.progressPercent(), 100);
    }
};

QTEST_MAIN(TransferTaskTest)
#include "tst_transfer_task.moc"
