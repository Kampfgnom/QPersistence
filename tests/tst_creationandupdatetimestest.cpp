#include "tst_creationandupdatetimestest.h"

CreationAndUpdateTimesTest::CreationAndUpdateTimesTest(QObject *parent) :
    QObject(parent)
{
}

void CreationAndUpdateTimesTest::init()
{
    QSKIP("This test does not work anymore");
}

#ifndef QP_NO_TIMESTAMPS
void CreationAndUpdateTimesTest::cleanupTestCase()
{
}

void CreationAndUpdateTimesTest::testCreationTime()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();

    VERIFY_QP_ERROR();

    QSqlQuery query("SELECT NOW()");
    QVERIFY(query.exec());
    QVERIFY(query.first());

    QDateTime creationTime = Qp::creationTimeInDatabase(parent);
    QDateTime updateTime = Qp::updateTimeInDatabase(parent);
    QDateTime databaseTime = query.value(0).toDateTime();

    QCOMPARE(creationTime, databaseTime);
    QCOMPARE(updateTime, databaseTime);
}

void CreationAndUpdateTimesTest::testUpdateTime()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();

    VERIFY_QP_ERROR();

    QDateTime creationTime = Qp::creationTimeInDatabase(parent);

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1010);
    Qp::update(parent);

    QSqlQuery query("SELECT NOW()");
    QVERIFY(query.exec());
    QVERIFY(query.first());

    QDateTime creationTimeAfterUpdate = Qp::creationTimeInDatabase(parent);
    QDateTime updateTime = Qp::updateTimeInDatabase(parent);
    QDateTime databaseTime = query.value(0).toDateTime();

    QCOMPARE(creationTime, creationTimeAfterUpdate);
    QVERIFY(updateTime != creationTime);
    QCOMPARE(updateTime, databaseTime);
    QVERIFY(updateTime >= creationTime.addSecs(1));
}

void CreationAndUpdateTimesTest::VERIFY_QP_ERROR()
{
    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}
#endif
