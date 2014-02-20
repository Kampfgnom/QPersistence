#include "tst_creationandupdatetimestest.h"

#include <QPersistence.h>
#include <QSqlError>
#include <QSqlQuery>

#include "relationtestbase.h"
#include "parentobject.h"
#include "childobject.h"

CreationAndUpdateTimesTest::CreationAndUpdateTimesTest(QObject *parent) :
    QObject(parent)
{
}

#ifndef QP_LOCALDB
void CreationAndUpdateTimesTest::initTestCase()
{
    RelationTestBase::initDatabase();
}

void CreationAndUpdateTimesTest::cleanupTestCase()
{
}

void CreationAndUpdateTimesTest::testCreationTime()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
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
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    VERIFY_QP_ERROR();

    QDateTime creationTime = Qp::creationTimeInDatabase(parent);

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);
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
    QCOMPARE(updateTime, creationTime.addSecs(1));
}

void CreationAndUpdateTimesTest::VERIFY_QP_ERROR()
{
    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}
#endif
