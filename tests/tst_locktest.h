#ifndef TST_LOCKTEST_H
#define TST_LOCKTEST_H

#include "tests_common.h"

#include "tst_synchronizetest.h"

class LockTest : public QObject
{
    Q_OBJECT
public:
    explicit LockTest(QObject *parent = 0);

#ifndef QP_NO_LOCKS
    static void cleanup(QProcess *process);

    void testSynchronizedCounter();

    static QHash<QString, QVariant> additionalLockInfo()
    {
        QHash<QString, QVariant> info;
        info.insert("aString", "asdagasg");
        info.insert("someNUmber", 123415);
        info.insert("date", QDate(123,12,24));
        return info;
    }

private slots:
    void startProcess();

    void testLockAndUnlockLocally();
    void testLockAndUnlockRemotely();
    void testLockRemotelyAndUnlockLocally();

    void testLockInformationLocal();
    void testLockInformationRemote();

private:
    QProcess *startChangerProcess(int id, SynchronizeTest::ChangerMode mode);
    static QProcess *m_currentProcess;
#endif
};

#endif // TST_LOCKTEST_H
