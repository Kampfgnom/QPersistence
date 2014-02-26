#include "tst_enumerationtest.h"

#include "../src/cache.h"

#include <QSqlError>

EnumerationTest::EnumerationTest(QObject *parent) :
    QObject(parent)
{
}

void EnumerationTest::initTestCase()
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

void EnumerationTest::testInitialEnumValue()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testenum+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::InitialValue));
}

void EnumerationTest::testUpdateEnum()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
    p->setTestEnum(TestNameSpace::ParentObject::Value2);
    Qp::update(p);

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testenum+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::Value2));
}

void EnumerationTest::testExplicitValue()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
    p->setTestEnum(TestNameSpace::ParentObject::ExplicitValue);
    Qp::update(p);

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testenum+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());

    QMetaProperty property = p->metaObject()->property(p->metaObject()->indexOfProperty("testEnum"));
    QCOMPARE(property.enumerator().value(query.value(0).toInt()), static_cast<int>(TestNameSpace::ParentObject::ExplicitValue));
}

void EnumerationTest::testReadEnum()
{
    int id = 0;
    {
        QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();
        p->setTestEnum(TestNameSpace::ParentObject::ValueAfterExplicitValue);
        Qp::update(p);
        id = Qp::primaryKey(p);

        Qp::dataAccessObject<TestNameSpace::ParentObject>()->cache().remove(id);
        p.clear();
    }

    QSharedPointer<TestNameSpace::ParentObject> p = Qp::read<TestNameSpace::ParentObject>(id);
    QCOMPARE(p->testEnum(), TestNameSpace::ParentObject::ValueAfterExplicitValue);
}
