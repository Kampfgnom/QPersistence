#include "tst_locktest.h"

#include <QSqlError>
#include <QTimer>
#include "relationtestbase.h"

LockTest::LockTest(QObject *parent) :
    QObject(parent)
{
}

#ifndef QP_LOCALDB
QProcess *LockTest::m_currentProcess(nullptr);

void LockTest::cleanup(QProcess *process)
{
    Q_ASSERT(process == m_currentProcess);
    Q_ASSERT(process->exitCode() == QProcess::NormalExit);
    Q_ASSERT(process->error() == QProcess::UnknownError);

    m_currentProcess->close();
    delete m_currentProcess;
    m_currentProcess = nullptr;
}

void LockTest::initTestCase()
{
    RelationTestBase::initDatabase();
}

void LockTest::testLockAndUnlockLocally()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

    QpLock lock = Qp::tryLock(parent);
    QCOMPARE(qSharedPointerCast<ParentObject>(lock.object()), parent);
    QCOMPARE(lock.status(), QpLock::LockedLocally);

    QpLock lockStatus = Qp::isLocked(parent);
    QCOMPARE(qSharedPointerCast<ParentObject>(lockStatus.object()), parent);
    QCOMPARE(lockStatus.status(), QpLock::LockedLocally);

    QpLock unlock = Qp::unlock(parent);
    QCOMPARE(qSharedPointerCast<ParentObject>(unlock.object()), parent);
    QCOMPARE(unlock.status(), QpLock::Unlocked);

    QpLock unlock2 = Qp::unlock(parent);
    QCOMPARE(qSharedPointerCast<ParentObject>(unlock2.object()), parent);
    QCOMPARE(unlock2.status(), QpLock::Unlocked);
}

void LockTest::testLockAndUnlockRemotely()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QScopedPointer<QProcess, LockTest> process(startChangerProcess(Qp::primaryKey(parent), SynchronizeTest::LockAndUnlock));

    QTRY_COMPARE(Qp::isLocked(parent).status(), QpLock::LockedRemotely);
    QCOMPARE(Qp::tryLock(parent).status(), QpLock::LockedRemotely);
    QTRY_COMPARE(Qp::isLocked(parent).status(), QpLock::Unlocked);
}

void LockTest::testLockRemotelyAndUnlockLocally()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QScopedPointer<QProcess, LockTest> process(startChangerProcess(Qp::primaryKey(parent), SynchronizeTest::LockAndUnlock));

    QTRY_COMPARE(Qp::isLocked(parent).status(), QpLock::LockedRemotely);
    QCOMPARE(Qp::unlock(parent).status(), QpLock::Unlocked);
    QCOMPARE(Qp::isLocked(parent).status(), QpLock::Unlocked);
}


void LockTest::testSynchronizedCounter()
{
    QFAIL("needs mysql server 5.6!");

    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QScopedPointer<QProcess, LockTest> process(startChangerProcess(Qp::primaryKey(parent), SynchronizeTest::LockedCounting));

    // Wait for the remote process to have started
    QTRY_COMPARE(Qp::isLocked(parent).status(), QpLock::LockedRemotely);

    for(int i = 0; i < 100; ++i) {
        QTRY_COMPARE(Qp::tryLock(parent).status(), QpLock::LockedLocally);
        Qp::synchronize(parent);
        parent->increaseCounter();
        Qp::update(parent);
        Qp::unlock(parent);
    }

    QVERIFY(process->waitForFinished());
    Qp::synchronize(parent);
    QCOMPARE(parent->counter(), 200);
}

void LockTest::startProcess()
{
    if(m_currentProcess)
        m_currentProcess->start();
}

QProcess *LockTest::startChangerProcess(int id, SynchronizeTest::ChangerMode mode)
{
    m_currentProcess = new QProcess(this);
    m_currentProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    m_currentProcess->setProgram("../testDatabaseChanger/qpersistencetestdatabasechanger");
    m_currentProcess->setArguments(QStringList()
                                   << QString("%1").arg(id)
                                   << QString("%1").arg(mode));

    m_currentProcess->start();
    return m_currentProcess;
}

#endif
