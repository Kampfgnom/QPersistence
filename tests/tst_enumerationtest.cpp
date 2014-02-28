#include "tst_enumerationtest.h"

#include "../src/cache.h"

EnumerationTest::EnumerationTest(QObject *parent) :
    QObject(parent)
{
}

void EnumerationTest::testInitialEnumValue()
{
    QSharedPointer<TestNameSpace::ParentObject> p = Qp::create<TestNameSpace::ParentObject>();

    QpSqlQuery query(Qp::database());
    query.prepare(QString("SELECT testenum+0 FROM parentobject WHERE _qp_id = %1")
                  .arg(Qp::primaryKey(p)));
    QVERIFY(query.exec());
    QVERIFY(query.first());

#ifdef QP_FOR_MYSQL
    QMetaProperty property = p->metaObject()->property(p->metaObject()->indexOfProperty("testEnum"));
    QCOMPARE(property.enumerator().value(query.value(0).toInt()), static_cast<int>(TestNameSpace::ParentObject::InitialValue));
#elif defined QP_FOR_SQLITE
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::InitialValue));
#endif
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

#ifdef QP_FOR_MYSQL
    QMetaProperty property = p->metaObject()->property(p->metaObject()->indexOfProperty("testEnum"));
    QCOMPARE(property.enumerator().value(query.value(0).toInt()), static_cast<int>(TestNameSpace::ParentObject::Value2));
#elif defined QP_FOR_SQLITE
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::Value2));
#endif
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

#ifdef QP_FOR_MYSQL
    QMetaProperty property = p->metaObject()->property(p->metaObject()->indexOfProperty("testEnum"));
    QCOMPARE(property.enumerator().value(query.value(0).toInt()), static_cast<int>(TestNameSpace::ParentObject::ExplicitValue));
#elif defined QP_FOR_SQLITE
    QCOMPARE(query.value(0).toInt(), static_cast<int>(TestNameSpace::ParentObject::ExplicitValue));
#endif
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
