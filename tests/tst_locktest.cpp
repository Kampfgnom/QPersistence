#include "tst_locktest.h"

#include <QSqlError>

LockTest::LockTest(QObject *parent) :
    QObject(parent)
{
}

void LockTest::initTestCase()
{
    QSqlDatabase db = Qp::database();
    if(!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("192.168.100.2");
        db.setDatabaseName("niklas");
        db.setUserName("niklas");
        db.setPassword("niklas");

        QVERIFY2(db.open(), db.lastError().text().toUtf8());

        Qp::enableLocks();
        Qp::setDatabase(db);
        Qp::setSqlDebugEnabled(true);
        Qp::registerClass<ParentObject>();
        Qp::registerClass<ChildObject>();
        Qp::createCleanSchema();
    }
}

void LockTest::testLockLocally()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

    QpLock lock = Qp::tryLock(parent);
    QCOMPARE(qSharedPointerCast<ParentObject>(lock.object()), parent);
    QCOMPARE(lock.status(), QpLock::LockedLocally);
}
