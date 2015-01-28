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

#ifndef QP_NO_TIMESTAMPS
void OneToManyRelationTest::testUpdateTimesFromParent()
{
    QSKIP("This test does not work anymore");

    static const int CHILDCOUNT = 3;
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    QDateTime timeParent = Qp::updateTimeInDatabase(parent);
    QDateTime timeChildren = Qp::updateTimeInDatabase(parent->childObjectsOneToMany().first());
    QCOMPARE(timeParent, timeChildren);

    foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsOneToMany()) {
        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeParent, timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1010);
    Qp::update(parent);

    // verify only parent time is changed, when no relation changes
    timeParent = Qp::updateTimeInDatabase(parent);
    foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsOneToMany()) {
        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeParent.addSecs(-1), timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1010);

    // change one child's parent
    QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> changedChild = parent->childObjectsOneToMany().first();
    parent2->addChildObjectsOneToMany(changedChild);
    Qp::update(parent2);

    {
        // verify both parent's times changed
        QDateTime newTimeParent1 = Qp::updateTimeInDatabase(parent);
        QDateTime newTimeParent2 = Qp::updateTimeInDatabase(parent2);
        QCOMPARE(newTimeParent1, timeParent.addSecs(1));
        QCOMPARE(newTimeParent1, newTimeParent2);

        // verify only the changed child's time changed
        QCOMPARE(newTimeParent1, Qp::updateTimeInDatabase(changedChild));

        foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsOneToMany()) {
            QDateTime timeChild = Qp::updateTimeInDatabase(child);
            QCOMPARE(timeChild, timeChildren);
        }
    }
}

void OneToManyRelationTest::testUpdateTimesFromChild()
{
    QSKIP("This test does not work anymore");

    static const int CHILDCOUNT = 3;
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        parent->addChildObjectsOneToMany(child);
        children.append(child);
        Qp::update(child);
    }

    QDateTime timeParent = Qp::updateTimeInDatabase(parent);
    QDateTime timeChildren = Qp::updateTimeInDatabase(parent->childObjectsOneToMany().first());
    QCOMPARE(timeParent, timeChildren);

    foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsOneToMany()) {
        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeParent, timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1010);
    Qp::update(parent->childObjectsOneToMany().first());

    // verify child updatetime changes
    QDateTime timeFirstChild = Qp::updateTimeInDatabase(parent->childObjectsOneToMany().first());
    QCOMPARE(timeFirstChild, timeChildren.addSecs(1));

    // verify parent time does not change
    QCOMPARE(timeParent, Qp::updateTimeInDatabase(parent));

    // verify other children's time does not change
    foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsOneToMany()) {
        if(child == parent->childObjectsOneToMany().first())
            continue;

        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeFirstChild.addSecs(-1), timeChild);
    }

    // Create another parent
    QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();
    parent2->addChildObjectsOneToMany(Qp::create<TestNameSpace::ChildObject>());
    parent2->addChildObjectsOneToMany(Qp::create<TestNameSpace::ChildObject>());
    QDateTime timeParent2 = Qp::updateTimeInDatabase(parent2);






    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1010);

    // change one child's parent
    QSharedPointer<TestNameSpace::ChildObject> changedChild = parent->childObjectsOneToMany().first();
    parent2->addChildObjectsOneToMany(changedChild);
    Qp::update(changedChild);

    {
        // verify both parent's times changed
        QDateTime newTimeParent1 = Qp::updateTimeInDatabase(parent);
        QDateTime newTimeParent2 = Qp::updateTimeInDatabase(parent2);
        QCOMPARE(newTimeParent1, timeFirstChild.addSecs(1));
        QCOMPARE(newTimeParent1, newTimeParent2);

        // verify the changed child's time changed
        QCOMPARE(newTimeParent1, Qp::updateTimeInDatabase(changedChild));

        // Verify other children times are unchanged
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsOneToMany()) {
            Q_ASSERT(child != changedChild);

            QDateTime timeChild = Qp::updateTimeInDatabase(child);
            QCOMPARE(timeChild, timeChildren);
        }

        foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent2->childObjectsOneToMany()) {
            if(child == changedChild)
                continue;

            QDateTime timeChild = Qp::updateTimeInDatabase(child);
            QCOMPARE(timeChild, timeParent2);
        }
    }
}
#endif
