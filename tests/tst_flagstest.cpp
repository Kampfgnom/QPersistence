#include "tst_flagstest.h"

#include "../src/cache.h"

#include <QSqlError>

FlagsTest::FlagsTest(QObject *parent) :
    QObject(parent)
{
}

void FlagsTest::initTestCase()
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
}

void FlagsTest::testInitialValue()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testoptions+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::InitialOption));
}


void FlagsTest::testUpdate()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
    p->setTestOptions(TestNameSpace::ParentObject::Option2);
    Qp::update(p);

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testoptions+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::Option2));
}

void FlagsTest::testCombined()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
    p->setTestOptions(TestNameSpace::ParentObject::CombinedOption);
    Qp::update(p);

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testoptions+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::CombinedOption));
}

void FlagsTest::testCustomCombined()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
    p->setTestOptions(TestNameSpace::ParentObject::Option2 | TestNameSpace::ParentObject::InitialOption);
    Qp::update(p);

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testoptions+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::Option2 | TestNameSpace::ParentObject::InitialOption));
}


void FlagsTest::testRead()
{
    int id = 0;
    {
        QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
        p->setTestOptions(TestNameSpace::ParentObject::Option3 | TestNameSpace::ParentObject::Option2 | TestNameSpace::ParentObject::InitialOption);
        Qp::update(p);
        id = Qp::primaryKey(p);

        Qp::dataAccessObject<TestNameSpace::ParentObject>()->cache().remove(id);
        p.clear();
    }

    QSharedPointer<TestNameSpace::ParentObject> p = Qp::read<TestNameSpace::ParentObject>(id);
    QCOMPARE(p->testOptions(), TestNameSpace::ParentObject::Option3 | TestNameSpace::ParentObject::Option2 | TestNameSpace::ParentObject::InitialOption);
}

void FlagsTest::testReadUnknownValue()
{
    int id = 0;
    {
        QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
        p->setTestOptions(TestNameSpace::ParentObject::UnknownOption);
        Qp::update(p);
        id = Qp::primaryKey(p);

        Qp::dataAccessObject<TestNameSpace::ParentObject>()->cache().remove(id);
        p.clear();
    }

    QSharedPointer<TestNameSpace::ParentObject> p = Qp::read<TestNameSpace::ParentObject>(id);
    QCOMPARE(p->testOptions(), TestNameSpace::ParentObject::UnknownOption);

}
