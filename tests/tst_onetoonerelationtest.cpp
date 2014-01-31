#include "tst_onetoonerelationtest.h"

OneToOneRelationTest::OneToOneRelationTest(QObject *parent) :
    RelationTestBase(parent)
{
}

void OneToOneRelationTest::initTestCase()
{
    RelationTestBase::initDatabase();

    QpMetaObject metaObject = QpMetaObject::forClassName(ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObject");

    metaObject = QpMetaObject::forClassName(ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectOneToOne");
}

void OneToOneRelationTest::cleanupTestCase()
{
    Qp::database().close();
}

QVariant OneToOneRelationTest::childFK(QSharedPointer<ParentObject> parent)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    select.setWhereCondition(QpSqlCondition(m_childToParentRelation.columnName(),
                                            QpSqlCondition::EqualTo,
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

QVariant OneToOneRelationTest::parentFK(QSharedPointer<ChildObject> child)
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

void OneToOneRelationTest::testInitialDatabaseFKEmpty()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    QCOMPARE(childFK(parent), QVariant());
    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
}

void OneToOneRelationTest::testDatabaseFKInsertFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObject(child);
    Qp::update(parent);

    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(child)));
    QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKInsertFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObject(child);
    Qp::update(child);

    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(child)));
    QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKChangeFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObject(child);
    Qp::update(parent);

    QSharedPointer<ChildObject> newChild = Qp::create<ChildObject>();
    parent->setChildObject(newChild);
    Qp::update(parent);

    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(newChild)));
    QCOMPARE(parentFK(newChild), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKChangeFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObject(child);
    Qp::update(child);

    QSharedPointer<ChildObject> newChild = Qp::create<ChildObject>();
    parent->setChildObject(newChild);
    Qp::update(newChild);

    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(newChild)));
    QCOMPARE(parentFK(newChild), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKClearFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObject(child);
    Qp::update(parent);

    parent->setChildObject(QSharedPointer<ChildObject>());
    Qp::update(parent);

    QCOMPARE(childFK(parent), QVariant());
    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
}

void OneToOneRelationTest::testDatabaseFKClearFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObject(child);
    Qp::update(child);

    parent->setChildObject(QSharedPointer<ChildObject>());
    Qp::update(child);

    QCOMPARE(childFK(parent), QVariant());
    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
}

void OneToOneRelationTest::testDatabaseUpdateTimes()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
    QSharedPointer<ChildObject> child2 = Qp::create<ChildObject>();
    QSharedPointer<ChildObject> child3 = Qp::create<ChildObject>();

    parent->setChildObject(child);
    Qp::update(child);

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
