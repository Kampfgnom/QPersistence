#include "tst_onetomanyrelationtest.h"

OneToManyRelationTest::OneToManyRelationTest(QObject *parent) :
    QObject(parent)
{
}

void OneToManyRelationTest::initTestCase()
{
    QpMetaObject metaObject = QpMetaObject::forClassName(TestNameSpace::ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObjectsOneToMany");

    metaObject = QpMetaObject::forClassName(TestNameSpace::ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectOneToMany");
}

void OneToManyRelationTest::testOneToManyRelation()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    parent->setObjectName("P1");
    QList<QSharedPointer<TestNameSpace::ChildObject>> shouldHaveChildren;

    // Add children to P1
    for(int i = 0; i < 6; ++i) {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        child->setObjectName(QString("P1_C%1").arg(i));
        parent->addHasMany(child);
        shouldHaveChildren.append(child);
    }

    // Verify tree
    {
        QList<QSharedPointer<TestNameSpace::ChildObject>> hasChildren = parent->hasMany();
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, shouldHaveChildren) {
            QVERIFY(hasChildren.contains(child));
            QCOMPARE(child->belongsToOneMany(), parent);
            hasChildren.removeOne(child);
        }
        QVERIFY(hasChildren.isEmpty());
    }

    // Remove children
    {
        QList<QSharedPointer<TestNameSpace::ChildObject>> removedChildren;
        removedChildren.append(shouldHaveChildren.takeAt(1));
        removedChildren.append(shouldHaveChildren.takeAt(2));
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, removedChildren) {
            parent->removeHasMany(child);
        }

        QList<QSharedPointer<TestNameSpace::ChildObject>> hasChildren = parent->hasMany();
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, shouldHaveChildren) {
            QVERIFY(hasChildren.contains(child));
            QCOMPARE(child->belongsToOneMany(), parent);
            hasChildren.removeOne(child);
        }
        QVERIFY(hasChildren.isEmpty());

        foreach(QSharedPointer<TestNameSpace::ChildObject> child, removedChildren) {
            QCOMPARE(child->belongsToOneMany(), QSharedPointer<TestNameSpace::ParentObject>());
        }
    }

    // Move children to other parent
    {
        QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();
        parent2->setObjectName("P2");

        QList<QSharedPointer<TestNameSpace::ChildObject>> shouldHaveChildren2;
        QList<QSharedPointer<TestNameSpace::ChildObject>> movedChildren;
        movedChildren.append(shouldHaveChildren.takeAt(1));
        movedChildren.append(shouldHaveChildren.takeAt(2));
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, movedChildren) {
            parent2->addHasMany(child);
            shouldHaveChildren2.append(child);
        }

        // verify parent1 children
        {
            QList<QSharedPointer<TestNameSpace::ChildObject>> hasChildren = parent->hasMany();
            foreach(QSharedPointer<TestNameSpace::ChildObject> child, shouldHaveChildren) {
                QVERIFY(hasChildren.contains(child));
                QCOMPARE(child->belongsToOneMany(), parent);
                hasChildren.removeOne(child);
            }
            QVERIFY(hasChildren.isEmpty());
        }

        // verify parent2 children
        {
            QList<QSharedPointer<TestNameSpace::ChildObject>> hasChildren = parent2->hasMany();
            foreach(QSharedPointer<TestNameSpace::ChildObject> child, shouldHaveChildren2) {
                QVERIFY(hasChildren.contains(child));
                QCOMPARE(child->belongsToOneMany(), parent2);
                hasChildren.removeOne(child);
            }
            QVERIFY(hasChildren.isEmpty());
        }
    }
}

QVariantList OneToManyRelationTest::childFKs(QSharedPointer<TestNameSpace::ParentObject> parent)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    select.setWhereCondition(QpCondition(m_childToParentRelation.columnName(),
                                            QpCondition::EqualTo,
                                            Qp::primaryKey(parent)));
    select.prepareSelect();

    if(!select.exec()) {
        return QVariantList();
    }

    QVariantList result;
    while(select.next()) {
        result.append(select.value(0));
    }
    return result;
}

QVariant OneToManyRelationTest::parentFK(QSharedPointer<TestNameSpace::ChildObject> child)
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

void OneToManyRelationTest::testParentFk(QSharedPointer<TestNameSpace::ChildObject> child)
{
    QSharedPointer<TestNameSpace::ParentObject> parent = child->parentObjectOneToMany();

    if(!parent)
        QCOMPARE(parentFK(child), NULLKEY());

    if(parent)
        QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
    else
        QCOMPARE(parentFK(child), NULLKEY());
}

void OneToManyRelationTest::testChildFks(QSharedPointer<TestNameSpace::ParentObject> parent)
{
    QVariantList fks = childFKs(parent);

    foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsOneToMany()) {
        QVariant pk = Qp::primaryKey(child);
        QVERIFY(fks.contains(pk));
        fks.removeAll(pk);
    }

    QVERIFY(fks.isEmpty());
}

void OneToManyRelationTest::testInitialDatabaseFKEmpty()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

    QCOMPARE(childFKs(parent), QVariantList());
    QCOMPARE(parentFK(child), NULLKEY());
}

void OneToManyRelationTest::testDatabaseFKInsertFromParent()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    testChildFks(parent);
    foreach(QSharedPointer<TestNameSpace::ChildObject> child, children) {
        testParentFk(child);
    }
}

void OneToManyRelationTest::testDatabaseFKInsertFromChild()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        Qp::update(child);
        children.append(child);
    }

    testChildFks(parent);
    foreach(QSharedPointer<TestNameSpace::ChildObject> child, children) {
        testParentFk(child);
    }
}

void OneToManyRelationTest::testDatabaseFKChangeFromParent()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    // Add new child
    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        children.append(child);
        Qp::update(parent);

        testChildFks(parent);
        foreach(QSharedPointer<TestNameSpace::ChildObject> c, children) {
            testParentFk(c);
        }
    }

    // Remove child
    {
        QSharedPointer<TestNameSpace::ChildObject> child = parent->childObjectsOneToMany().first();
        parent->removeChildObjectsOneToMany(child);
        Qp::update(parent);

        testChildFks(parent);
        foreach(QSharedPointer<TestNameSpace::ChildObject> c, children) {
            testParentFk(c);
        }
    }

    QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();

    // Move child to another parent
    {
        parent2->addChildObjectsOneToMany(parent->childObjectsOneToMany().first());
        Qp::update(parent2);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, children) {
            testParentFk(child);
        }
    }

    // Move another child to another parent
    {
        parent2->addChildObjectsOneToMany(parent->childObjectsOneToMany().last());
        Qp::update(parent2);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, children) {
            testParentFk(child);
        }
    }
}

void OneToManyRelationTest::testDatabaseFKChangeFromChild()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    // Add new child
    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        children.append(child);
        Qp::update(child);

        testChildFks(parent);
        foreach(QSharedPointer<TestNameSpace::ChildObject> c, children) {
            testParentFk(c);
        }
    }

    // Remove child
    {
        QSharedPointer<TestNameSpace::ChildObject> child = parent->childObjectsOneToMany().first();
        Qp::synchronize(child);
        parent->removeChildObjectsOneToMany(child);
        Qp::update(child);

        testChildFks(parent);
        foreach(QSharedPointer<TestNameSpace::ChildObject> c, children) {
            testParentFk(c);
        }
    }

    QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();

    // Move child to another parent
    {
        QSharedPointer<TestNameSpace::ChildObject> child = parent->childObjectsOneToMany().first();
        Qp::synchronize(child);
        parent2->addChildObjectsOneToMany(child);
        Qp::update(child);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<TestNameSpace::ChildObject> c, children) {
            testParentFk(c);
        }
    }

    // Move another child to another parent
    {
        QSharedPointer<TestNameSpace::ChildObject> child = parent->childObjectsOneToMany().last();
        Qp::synchronize(child);
        parent2->addChildObjectsOneToMany(child);
        Qp::update(child);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<TestNameSpace::ChildObject> c, children) {
            testParentFk(c);
        }
    }
}

