#include "tst_onetomanyrelationtest.h"

OneToManyRelationTest::OneToManyRelationTest(QObject *parent) :
    RelationTestBase(parent)
{
}

void OneToManyRelationTest::initTestCase()
{
    RelationTestBase::initDatabase();

    QpMetaObject metaObject = QpMetaObject::forClassName(ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObjectsOneToMany");

    metaObject = QpMetaObject::forClassName(ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectOneToMany");
}

void OneToManyRelationTest::cleanupTestCase()
{
}

QVariantList OneToManyRelationTest::childFKs(QSharedPointer<ParentObject> parent)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    select.setWhereCondition(QpSqlCondition(m_childToParentRelation.columnName(),
                                            QpSqlCondition::EqualTo,
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

QVariant OneToManyRelationTest::parentFK(QSharedPointer<ChildObject> child)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(m_childToParentRelation.columnName());
    select.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                            QpSqlCondition::EqualTo,
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

void OneToManyRelationTest::testParentFk(QSharedPointer<ChildObject> child)
{
    QSharedPointer<ParentObject> parent = child->parentObjectOneToMany();

    if(!parent)
        QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));

    if(parent)
        QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
    else
        QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
}

void OneToManyRelationTest::testChildFks(QSharedPointer<ParentObject> parent)
{
    QVariantList fks = childFKs(parent);

    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        QVariant pk = Qp::primaryKey(child);
        QVERIFY(fks.contains(pk));
        fks.removeAll(pk);
    }

    QVERIFY(fks.isEmpty());
}

void OneToManyRelationTest::testInitialDatabaseFKEmpty()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    QCOMPARE(childFKs(parent), QVariantList());
    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
}

void OneToManyRelationTest::testDatabaseFKInsertFromParent()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    testChildFks(parent);
    foreach(QSharedPointer<ChildObject> child, children) {
        testParentFk(child);
    }
}

void OneToManyRelationTest::testDatabaseFKInsertFromChild()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        Qp::update(child);
        children.append(child);
    }

    testChildFks(parent);
    foreach(QSharedPointer<ChildObject> child, children) {
        testParentFk(child);
    }
}

void OneToManyRelationTest::testDatabaseFKChangeFromParent()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    // Add new child
    {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        children.append(child);
        Qp::update(parent);

        testChildFks(parent);
        foreach(QSharedPointer<ChildObject> child, children) {
            testParentFk(child);
        }
    }

    // Remove child
    {
        QSharedPointer<ChildObject> child = parent->childObjectsOneToMany().first();
        parent->removeChildObjectOneToMany(child);
        Qp::update(parent);

        testChildFks(parent);
        foreach(QSharedPointer<ChildObject> c, children) {
            testParentFk(c);
        }
    }

    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();

    // Move child to another parent
    {
        parent2->addChildObjectOneToMany(parent->childObjectsOneToMany().first());
        Qp::update(parent2);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<ChildObject> child, children) {
            testParentFk(child);
        }
    }

    // Move another child to another parent
    {
        parent2->addChildObjectOneToMany(parent->childObjectsOneToMany().last());
        Qp::update(parent2);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<ChildObject> child, children) {
            testParentFk(child);
        }
    }
}

void OneToManyRelationTest::testDatabaseFKChangeFromChild()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    // Add new child
    {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        children.append(child);
        Qp::update(child);

        testChildFks(parent);
        foreach(QSharedPointer<ChildObject> child, children) {
            testParentFk(child);
        }
    }

    // Remove child
    {
        QSharedPointer<ChildObject> child = parent->childObjectsOneToMany().first();
        parent->removeChildObjectOneToMany(child);
        Qp::update(child);

        testChildFks(parent);
        foreach(QSharedPointer<ChildObject> c, children) {
            testParentFk(c);
        }
    }

    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();

    // Move child to another parent
    {
        QSharedPointer<ChildObject> child = parent->childObjectsOneToMany().first();
        parent2->addChildObjectOneToMany(child);
        Qp::update(child);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<ChildObject> c, children) {
            testParentFk(c);
        }
    }

    // Move another child to another parent
    {
        QSharedPointer<ChildObject> child = parent->childObjectsOneToMany().last();
        parent2->addChildObjectOneToMany(child);
        Qp::update(child);

        testChildFks(parent);
        testChildFks(parent2);
        foreach(QSharedPointer<ChildObject> c, children) {
            testParentFk(c);
        }
    }
}

void OneToManyRelationTest::testUpdateTimesFromParent()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        children.append(child);
    }
    Qp::update(parent);

    QDateTime timeParent = Qp::updateTimeInDatabase(parent);
    QDateTime timeChildren = Qp::updateTimeInDatabase(parent->childObjectsOneToMany().first());
    QCOMPARE(timeParent, timeChildren);

    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeParent, timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);
    Qp::update(parent);

    // verify only parent time is changed, when no relation changes
    timeParent = Qp::updateTimeInDatabase(parent);
    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeParent.addSecs(-1), timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);

    // change one child's parent
    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> changedChild = parent->childObjectsOneToMany().first();
    parent2->addChildObjectOneToMany(changedChild);
    Qp::update(parent2);

    {
        // verify both parent's times changed
        QDateTime newTimeParent1 = Qp::updateTimeInDatabase(parent);
        QDateTime newTimeParent2 = Qp::updateTimeInDatabase(parent2);
        QCOMPARE(newTimeParent1, timeParent.addSecs(1));
        QCOMPARE(newTimeParent1, newTimeParent2);

        // verify only the changed child's time changed
        QCOMPARE(newTimeParent1, Qp::updateTimeInDatabase(changedChild));

        foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
            QDateTime timeChild = Qp::updateTimeInDatabase(child);
            QCOMPARE(timeChild, timeChildren);
        }
    }
}

void OneToManyRelationTest::testUpdateTimesFromChild()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        children.append(child);
        Qp::update(child);
    }

    QDateTime timeParent = Qp::updateTimeInDatabase(parent);
    QDateTime timeChildren = Qp::updateTimeInDatabase(parent->childObjectsOneToMany().first());
    QCOMPARE(timeParent, timeChildren);

    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeParent, timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);
    Qp::update(parent->childObjectsOneToMany().first());

    // verify child updatetime changes
    QDateTime timeFirstChild = Qp::updateTimeInDatabase(parent->childObjectsOneToMany().first());
    QCOMPARE(timeFirstChild, timeChildren.addSecs(1));

    // verify parent time does not change
    QCOMPARE(timeParent, Qp::updateTimeInDatabase(parent));

    // verify other children's time does not change
    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        if(child == parent->childObjectsOneToMany().first())
            continue;

        QDateTime timeChild = Qp::updateTimeInDatabase(child);
        QCOMPARE(timeFirstChild.addSecs(-1), timeChild);
    }

    // Create another parent
    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    parent2->addChildObjectOneToMany(Qp::create<ChildObject>());
    parent2->addChildObjectOneToMany(Qp::create<ChildObject>());
    QDateTime timeParent2 = Qp::updateTimeInDatabase(parent2);






    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);

    // change one child's parent
    QSharedPointer<ChildObject> changedChild = parent->childObjectsOneToMany().first();
    parent2->addChildObjectOneToMany(changedChild);
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
        foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
            Q_ASSERT(child != changedChild);

            QDateTime timeChild = Qp::updateTimeInDatabase(child);
            QCOMPARE(timeChild, timeChildren);
        }

        foreach(QSharedPointer<ChildObject> child, parent2->childObjectsOneToMany()) {
            if(child == changedChild)
                continue;

            QDateTime timeChild = Qp::updateTimeInDatabase(child);
            QCOMPARE(timeChild, timeParent2);
        }
    }
}
