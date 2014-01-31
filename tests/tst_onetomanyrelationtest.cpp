#include "tst_onetomanyrelationtest.h"

OneToManyRelationTest::OneToManyRelationTest(QObject *parent) :
    RelationTestBase(parent)
{
}

void OneToManyRelationTest::initTestCase()
{
    RelationTestBase::initDatabase();

    QpMetaObject metaObject = QpMetaObject::forClassName(ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObjects");

    metaObject = QpMetaObject::forClassName(ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectOneToMany");
}

void OneToManyRelationTest::cleanupTestCase()
{
    Qp::database().close();
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

    QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
}

void OneToManyRelationTest::testChildFks(QSharedPointer<ParentObject> parent)
{
    QVariantList fks = childFKs(parent);

    foreach(QSharedPointer<ChildObject> child, parent->childObjects()) {
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
        parent->addChildObject(child);
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
        parent->addChildObject(child);
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
        parent->addChildObject(child);
        children.append(child);
    }
    Qp::update(parent);

    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    parent2->addChildObject(parent->childObjects().first());
    Qp::update(parent2);

    testChildFks(parent);
    testChildFks(parent2);
    foreach(QSharedPointer<ChildObject> child, children) {
        testParentFk(child);
    }

    parent2->addChildObject(parent->childObjects().last());
    Qp::update(parent2);

    testChildFks(parent);
    testChildFks(parent2);
    foreach(QSharedPointer<ChildObject> child, children) {
        testParentFk(child);
    }
}

void OneToManyRelationTest::testDatabaseFKChangeFromChild()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObject(child);
        children.append(child);
    }
    Qp::update(parent);

    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> changedChild = parent->childObjects().first();
    parent2->addChildObject(changedChild);
    Qp::update(changedChild);

    testChildFks(parent);
    testChildFks(parent2);
    foreach(QSharedPointer<ChildObject> child, children) {
        testParentFk(child);
    }

    changedChild = parent->childObjects().last();
    parent2->addChildObject(changedChild);
    Qp::update(changedChild);

    testChildFks(parent);
    testChildFks(parent2);
    foreach(QSharedPointer<ChildObject> child, children) {
        testParentFk(child);
    }
}


void OneToManyRelationTest::testUpdateTimesFromParent()
{
    static const int CHILDCOUNT = 3;
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QList<QSharedPointer<ChildObject>> children;
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObject(child);
        children.append(child);
    }
    Qp::update(parent);

    QDateTime timeParent = Qp::updateTime(parent);
    QDateTime timeChildren = Qp::updateTime(parent->childObjects().first());
    QCOMPARE(timeParent, timeChildren);

    foreach(QSharedPointer<ChildObject> child, parent->childObjects()) {
        QDateTime timeChild = Qp::updateTime(child);
        QCOMPARE(timeParent, timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);
    Qp::update(parent);

    // verify only parent time is changed, when no relation changes
    timeParent = Qp::updateTime(parent);
    foreach(QSharedPointer<ChildObject> child, parent->childObjects()) {
        QDateTime timeChild = Qp::updateTime(child);
        QCOMPARE(timeParent.addSecs(-1), timeChild);
    }

    qDebug() << "Sleeping 1 second...";
    QTest::qSleep(1000);

    // change one child's parent
    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> changedChild = parent->childObjects().first();
    parent2->addChildObject(changedChild);
    Qp::update(parent2);

    {
        // verify both parent's times changed
        QDateTime newTimeParent1 = Qp::updateTime(parent);
        QDateTime newTimeParent2 = Qp::updateTime(parent2);
        QCOMPARE(newTimeParent1, timeParent.addSecs(1));
        QCOMPARE(newTimeParent1, newTimeParent2);

        // verify only the changed child's time changed
        QCOMPARE(newTimeParent1, Qp::updateTime(changedChild));

        foreach(QSharedPointer<ChildObject> child, parent->childObjects()) {
            QDateTime timeChild = Qp::updateTime(child);
            QCOMPARE(timeChild, timeChildren);
        }
    }

}
