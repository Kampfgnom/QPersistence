#include "tst_creationandupdatetimestest.h"

#include "parentobject.h"
#include "childobject.h"

#include <QPersistence.h>

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
#include <QSqlQuery>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

CreationAndUpdateTimesTest::CreationAndUpdateTimesTest(QObject *parent) :
    QObject(parent)
{
}

void CreationAndUpdateTimesTest::initTestCase()
{
    QSqlDatabase db = Qp::database();
    if(!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("192.168.100.2");
        db.setDatabaseName("niklas");
        db.setUserName("niklas");
        db.setPassword("niklas");

        QVERIFY2(db.open(), db.lastError().text().toUtf8());

        Qp::setDatabase(db);
        Qp::setSqlDebugEnabled(false);
        Qp::registerClass<TestNameSpace::ParentObject>();
        Qp::registerClass<TestNameSpace::ChildObject>();
        Qp::createCleanSchema();
    }

    VERIFY_QP_ERROR();
}

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
