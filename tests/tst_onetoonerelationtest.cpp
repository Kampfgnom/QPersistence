#include "tst_onetoonerelationtest.h"

OneToOneRelationTest::OneToOneRelationTest(QObject *parent) :
    RelationTestBase(parent)
{
}

void OneToOneRelationTest::initTestCase()
{
    RelationTestBase::initDatabase();

    QpMetaObject metaObject = QpMetaObject::forClassName(ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObjectOneToOne");

    metaObject = QpMetaObject::forClassName(ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectOneToOne");
}

void OneToOneRelationTest::cleanupTestCase()
{
}

void OneToOneRelationTest::testOneToOneRelation()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    parent->setObjectName("P1");
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
    child->setObjectName("C1");

    parent->setHasOne(child);

    QCOMPARE(parent->hasOne(), child);
    QCOMPARE(child->belongsToOne(), parent);

    // Add child to another parent
    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    parent2->setObjectName("P2");
    parent2->setHasOne(child);
    QCOMPARE(parent->hasOne(), QSharedPointer<ChildObject>());
    QCOMPARE(parent2->hasOne(), child);
    QCOMPARE(child->belongsToOne(), parent2);

    // Add another child to the parent
    QSharedPointer<ChildObject> child2 = Qp::create<ChildObject>();
    child2->setObjectName("C2");
    parent2->setHasOne(child2);
    QCOMPARE(parent->hasOne(), QSharedPointer<ChildObject>());
    QCOMPARE(parent2->hasOne(), child2);
    QCOMPARE(child->belongsToOne(), QSharedPointer<ParentObject>());
    QCOMPARE(child2->belongsToOne(), parent2);
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

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(child)));
    QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKInsertFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(child)));
    QCOMPARE(parentFK(child), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKChangeFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    QSharedPointer<ChildObject> newChild = Qp::create<ChildObject>();
    parent->setChildObjectOneToOne(newChild);
    Qp::update(parent);

    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(newChild)));
    QCOMPARE(parentFK(newChild), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKChangeFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    QSharedPointer<ChildObject> newChild = Qp::create<ChildObject>();
    parent->setChildObjectOneToOne(newChild);
    Qp::update(newChild);

    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
    QCOMPARE(childFK(parent), QVariant(Qp::primaryKey(newChild)));
    QCOMPARE(parentFK(newChild), QVariant(Qp::primaryKey(parent)));
}

void OneToOneRelationTest::testDatabaseFKClearFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    parent->setChildObjectOneToOne(QSharedPointer<ChildObject>());
    Qp::update(parent);

    QCOMPARE(childFK(parent), QVariant());
    QCOMPARE(parentFK(child), QVariant(QVariant().toInt()));
}

void OneToOneRelationTest::testDatabaseFKClearFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    parent->setChildObjectOneToOne(QSharedPointer<ChildObject>());
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

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    QDateTime updateTimeParent = Qp::updateTimeInDatabase(parent);
    QDateTime updateTimeChild = Qp::updateTimeInDatabase(child);

    QCOMPARE(updateTimeParent, updateTimeChild);

    {
        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        Qp::update(parent);
        QDateTime updateTimeParentAfterParentUpdate = Qp::updateTimeInDatabase(parent);
        QDateTime updateTimeChildAfterParentUpdate = Qp::updateTimeInDatabase(child);

        QCOMPARE(updateTimeParentAfterParentUpdate, updateTimeParent.addSecs(1));
        QCOMPARE(updateTimeChildAfterParentUpdate, updateTimeChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        Qp::update(child);
        QDateTime updateTimeParentAfterChildUpdate = Qp::updateTimeInDatabase(parent);
        QDateTime updateTimeChildAfterChildUpdate = Qp::updateTimeInDatabase(child);

        QCOMPARE(updateTimeParentAfterChildUpdate, updateTimeParentAfterParentUpdate);
        QCOMPARE(updateTimeChildAfterChildUpdate, updateTimeChild.addSecs(2));
    }

    {
        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        parent->setChildObjectOneToOne(child2);
        Qp::update(parent);

        QDateTime updateTimeParentAfterParentUpdate = Qp::updateTimeInDatabase(parent);
        QDateTime updateTimeChild2AfterParentUpdate = Qp::updateTimeInDatabase(child2);
        QDateTime updateTimeChildAfterParentUpdate = Qp::updateTimeInDatabase(child);

        QCOMPARE(updateTimeParentAfterParentUpdate, updateTimeChild2AfterParentUpdate);
        QCOMPARE(updateTimeChild2AfterParentUpdate, updateTimeChildAfterParentUpdate);
    }

    {
        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        Qp::synchronize(child);
        parent->setChildObjectOneToOne(child);
        QCOMPARE(Qp::update(child), Qp::UpdateSuccess);

        QDateTime updateTimeParentAfterUpdate = Qp::updateTimeInDatabase(parent);
        QDateTime updateTimeChild2AfterUpdate = Qp::updateTimeInDatabase(child2);
        QDateTime updateTimeChildAfterUpdate = Qp::updateTimeInDatabase(child);

        QCOMPARE(updateTimeParentAfterUpdate, updateTimeChild2AfterUpdate);
        QCOMPARE(updateTimeParentAfterUpdate, updateTimeChildAfterUpdate);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);
        Qp::synchronize(child3);
        parent->setChildObjectOneToOne(child3);
        QCOMPARE(Qp::update(child3), Qp::UpdateSuccess);

        QDateTime updateTimeParentAfterchild3Update = Qp::updateTimeInDatabase(parent);
        QDateTime updateTimeChild2Afterchild3Update = Qp::updateTimeInDatabase(child2);
        QDateTime updateTimeChild3Afterchild3Update = Qp::updateTimeInDatabase(child3);
        QDateTime updateTimeChildAfterchild3Update = Qp::updateTimeInDatabase(child);

        QCOMPARE(updateTimeParentAfterchild3Update, updateTimeChildAfterchild3Update);
        QCOMPARE(updateTimeParentAfterchild3Update, updateTimeChild3Afterchild3Update);
        QVERIFY(updateTimeChild2Afterchild3Update != updateTimeParentAfterchild3Update);
        QCOMPARE(updateTimeChild2Afterchild3Update, updateTimeChild2AfterUpdate);
    }
}
