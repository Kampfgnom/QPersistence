#include "tst_creationandupdatetimestest.h"

#include <QPersistence.h>
#include <QSqlError>
#include <QSqlQuery>

#include "parentobject.h"
#include "childobject.h"

CreationAndUpdateTimesTest::CreationAndUpdateTimesTest(QObject *parent) :
    QObject(parent)
{
}

void CreationAndUpdateTimesTest::initTestCase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("192.168.100.2");
    db.setDatabaseName("niklas");
    db.setUserName("niklas");
    db.setPassword("niklas");

    QVERIFY2(db.open(), db.lastError().text().toUtf8());

    Qp::setSqlDebugEnabled(true);
    Qp::setDatabase(db);
    Qp::registerClass<ParentObject>();
    Qp::registerClass<ChildObject>();
    Qp::createCleanSchema();

    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}

void CreationAndUpdateTimesTest::cleanupTestCase()
{
    Qp::database().close();
}

void CreationAndUpdateTimesTest::testCreationTime()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    VERIFY_QP_ERROR();

    QSqlQuery query("SELECT NOW()");
    QVERIFY(query.exec());
    QVERIFY(query.first());

    QDateTime creationTime = Qp::creationTime(parent);
    QDateTime updateTime = Qp::updateTime(parent);
    QDateTime databaseTime = query.value(0).toDateTime();

    QCOMPARE(creationTime, databaseTime);
    QCOMPARE(updateTime, databaseTime);
}

void CreationAndUpdateTimesTest::testUpdateTimeForOneObject()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    VERIFY_QP_ERROR();

    QDateTime creationTime = Qp::creationTime(parent);

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);
    Qp::update(parent);

    QSqlQuery query("SELECT NOW()");
    QVERIFY(query.exec());
    QVERIFY(query.first());

    QDateTime creationTimeAfterUpdate = Qp::creationTime(parent);
    QDateTime updateTime = Qp::updateTime(parent);
    QDateTime databaseTime = query.value(0).toDateTime();

    QCOMPARE(creationTime, creationTimeAfterUpdate);
    QVERIFY(updateTime != creationTime);
    QCOMPARE(updateTime, databaseTime);
    QCOMPARE(updateTime, creationTime.addSecs(1));
}

void CreationAndUpdateTimesTest::testUpdateTimeForOneToOneRelations()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
    QSharedPointer<ChildObject> child2 = Qp::create<ChildObject>();
    QSharedPointer<ChildObject> child3 = Qp::create<ChildObject>();
    VERIFY_QP_ERROR();

    parent->setChildObject(child);
    Qp::update(parent);

    QDateTime updateTimeParent = Qp::updateTime(parent);
    QDateTime updateTimeChild = Qp::updateTime(child);

    QCOMPARE(updateTimeParent, updateTimeChild);

    {
        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        Qp::update(parent);
        QDateTime updateTimeParentAfterParentUpdate = Qp::updateTime(parent);
        QDateTime updateTimeChildAfterParentUpdate = Qp::updateTime(child);

        QCOMPARE(updateTimeParentAfterParentUpdate, updateTimeParent.addSecs(1));
        QCOMPARE(updateTimeChildAfterParentUpdate, updateTimeChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        Qp::update(child);
        QDateTime updateTimeParentAfterChildUpdate = Qp::updateTime(parent);
        QDateTime updateTimeChildAfterChildUpdate = Qp::updateTime(child);

        QCOMPARE(updateTimeParentAfterChildUpdate, updateTimeParentAfterParentUpdate);
        QCOMPARE(updateTimeChildAfterChildUpdate, updateTimeChild.addSecs(2));
    }

    {
        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        parent->setChildObject(child2);
        Qp::update(parent);

        QDateTime updateTimeParentAfterParentUpdate = Qp::updateTime(parent);
        QDateTime updateTimeChild2AfterParentUpdate = Qp::updateTime(child2);
        QDateTime updateTimeChildAfterParentUpdate = Qp::updateTime(child);

        QCOMPARE(updateTimeParentAfterParentUpdate, updateTimeChild2AfterParentUpdate);
        QCOMPARE(updateTimeChild2AfterParentUpdate, updateTimeChildAfterParentUpdate);
    }

    {
        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        parent->setChildObject(child);
        Qp::update(child);

        QDateTime updateTimeParentAfterUpdate = Qp::updateTime(parent);
        QDateTime updateTimeChild2AfterUpdate = Qp::updateTime(child2);
        QDateTime updateTimeChildAfterUpdate = Qp::updateTime(child);

        QCOMPARE(updateTimeParentAfterUpdate, updateTimeChild2AfterUpdate);
        QCOMPARE(updateTimeChild2AfterUpdate, updateTimeChildAfterUpdate);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        parent->setChildObject(child3);
        Qp::update(child3);

        QDateTime updateTimeParentAfterchild3Update = Qp::updateTime(parent);
        QDateTime updateTimeChild2Afterchild3Update = Qp::updateTime(child2);
        QDateTime updateTimeChild3Afterchild3Update = Qp::updateTime(child3);
        QDateTime updateTimeChildAfterchild3Update = Qp::updateTime(child);

        QCOMPARE(updateTimeParentAfterchild3Update, updateTimeChildAfterchild3Update);
        QCOMPARE(updateTimeParentAfterchild3Update, updateTimeChild3Afterchild3Update);
        QVERIFY(updateTimeChild2Afterchild3Update != updateTimeParentAfterchild3Update);
        QCOMPARE(updateTimeChild2Afterchild3Update, updateTimeChild2AfterUpdate);
    }
}

void CreationAndUpdateTimesTest::testUpdateTimeForOneToManyRelations()
{
    const int CHILDCOUNT = 4;

    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObject(child);
    }
    Qp::update(parent);

    QDateTime updateTimeParent = Qp::updateTime(parent);
    QDateTime updateTimeChildren = Qp::updateTime(parent->childObjects().first());
    QCOMPARE(updateTimeParent, updateTimeChildren);

    foreach(QSharedPointer<ChildObject> child, parent->childObjects()) {
        QDateTime updateTimeChild = Qp::updateTime(child);
        QCOMPARE(updateTimeParent, updateTimeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);
    Qp::update(parent);

    updateTimeParent = Qp::updateTime(parent);
    foreach(QSharedPointer<ChildObject> child, parent->childObjects()) {
        QDateTime updateTimeChild = Qp::updateTime(child);
        QCOMPARE(updateTimeParent.addSecs(-1), updateTimeChild);
    }


}

void CreationAndUpdateTimesTest::testUpdateTimeForManyToManyRelations()
{
    const int CHILDCOUNT = 4;
    const int PARENTCOUNT = 3;

    // Insert a tree of related parents and childs
    QList<QSharedPointer<ParentObject>> parents;
    QList<QSharedPointer<ChildObject>> children;
    for(int j = 0; j < PARENTCOUNT; ++j) {
        QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
        foreach(QSharedPointer<ParentObject> parent2, parents) {
            foreach(QSharedPointer<ChildObject> child2, parent2->childObjects()) {
                parent->addChildObjectManyToMany(child2);
            }
        }

        for(int i = 0; i < CHILDCOUNT; ++i) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            children.append(child);
            parent->addChildObjectManyToMany(child);
        }

        parents.append(parent);
        Qp::update(parent);
    }

    {

    }
}

void CreationAndUpdateTimesTest::VERIFY_QP_ERROR()
{
    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}
