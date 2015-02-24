#include "tst_onetoonerelationtest.h"

OneToOneRelationTest::OneToOneRelationTest(QObject *parent) :
    QObject(parent)
{
}

void OneToOneRelationTest::initTestCase()
{
    QpMetaObject metaObject = QpMetaObject::forClassName(TestNameSpace::ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObjectOneToOne");

    metaObject = QpMetaObject::forClassName(TestNameSpace::ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectOneToOne");
}

void OneToOneRelationTest::testOneToOneRelation()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    parent->setObjectName("P1");
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
    child->setObjectName("C1");

    parent->setHasOne(child);

    QCOMPARE(parent->hasOne(), child);
    QCOMPARE(child->belongsToOne(), parent);

    // Add child to another parent
    QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();
    parent2->setObjectName("P2");
    parent2->setHasOne(child);
    QCOMPARE(parent->hasOne(), QSharedPointer<TestNameSpace::ChildObject>());
    QCOMPARE(parent2->hasOne(), child);
    QCOMPARE(child->belongsToOne(), parent2);

    // Add another child to the parent
    QSharedPointer<TestNameSpace::ChildObject> child2 = Qp::create<TestNameSpace::ChildObject>();
    child2->setObjectName("C2");
    parent2->setHasOne(child2);
    QCOMPARE(parent->hasOne(), QSharedPointer<TestNameSpace::ChildObject>());
    QCOMPARE(parent2->hasOne(), child2);
    QCOMPARE(child->belongsToOne(), QSharedPointer<TestNameSpace::ParentObject>());
    QCOMPARE(child2->belongsToOne(), parent2);
}

QVariant OneToOneRelationTest::childFK(QSharedPointer<TestNameSpace::ParentObject> parent)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    select.setWhereCondition(QpCondition(m_childToParentRelation.columnName(),
                                            QpCondition::EqualTo,
                                            Qp::primaryKey(parent)));
    select.prepareSelect();

    if(!select.exec()) {
        return QVariant();
    }

    if(!select.first()) {
        return QVariant();
    }

    return select.value(0);
}

QVariant OneToOneRelationTest::parentFK(QSharedPointer<TestNameSpace::ChildObject> child)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(m_childToParentRelation.columnName());
    select.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                            QpCondition::EqualTo,
                                            Qp::primaryKey(child)));
    select.prepareSelect();

    if(!select.exec()) {
        return QVariant();
    }

    if(!select.first()) {
        return QVariant();
    }

    return select.value(0);
}

void OneToOneRelationTest::testInitialDatabaseFKEmpty()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    QCOMPARE(childFK(parent), QVariant());
    QCOMPARE(parentFK(child), NULLKEY());
}

void OneToOneRelationTest::testDatabaseFKInsertFromParent()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(child)));
    QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKInsertFromChild()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(child)));
    QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKChangeFromParent()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    QSharedPointer<TestNameSpace::ChildObject> newChild = Qp::create<TestNameSpace::ChildObject>();
    parent->setChildObjectOneToOne(newChild);
    Qp::update(parent);

    QCOMPARE(parentFK(child), NULLKEY());
    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(newChild)));
    QCOMPARE(parentFK(newChild), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKChangeFromChild()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    QSharedPointer<TestNameSpace::ChildObject> newChild = Qp::create<TestNameSpace::ChildObject>();
    parent->setChildObjectOneToOne(newChild);
    Qp::update(newChild);

    QCOMPARE(parentFK(child), NULLKEY());
    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(newChild)));
    QCOMPARE(parentFK(newChild), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKClearFromParent()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    parent->setChildObjectOneToOne(QSharedPointer<TestNameSpace::ChildObject>());
    Qp::update(parent);

    QCOMPARE(childFK(parent), QVariant());
    QCOMPARE(parentFK(child), NULLKEY());
}

void OneToOneRelationTest::testDatabaseFKClearFromChild()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    parent->setChildObjectOneToOne(QSharedPointer<TestNameSpace::ChildObject>());
    Qp::update(child);

    QCOMPARE(childFK(parent), QVariant());
    QCOMPARE(parentFK(child), NULLKEY());
}

