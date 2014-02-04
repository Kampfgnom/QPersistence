#include "tst_manytomanyrelationstest.h"

ManyToManyRelationsTest::ManyToManyRelationsTest(QObject *parent) :
    RelationTestBase(parent)
{
}

void ManyToManyRelationsTest::initTestCase()
{
    RelationTestBase::initDatabase();

    QpMetaObject metaObject = QpMetaObject::forClassName(ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObjectsManyToMany");

    metaObject = QpMetaObject::forClassName(ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectsManyToMany");
}

void ManyToManyRelationsTest::cleanupTestCase()
{
}

void ManyToManyRelationsTest::testInitialDatabaseFKsEmpty()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    QCOMPARE(childFKs(parent), QVariantList());
    QCOMPARE(parentFKs(child), QVariantList());
}

void ManyToManyRelationsTest::testDatabaseFKInsertFromParent()
{
    Tree tree = createTree();
    testTree(tree);
}

void ManyToManyRelationsTest::testDatabaseFKInsertFromChild()
{
    // Need to create our own tree here, because createTree() updates the parents
    QList<QSharedPointer<ParentObject>> parents;
    QList<QSharedPointer<ChildObject>> children;
    for(int j = 0; j < PARENTCOUNT; ++j) {
        QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

        for(int i = 0; i < CHILDCOUNT; ++i) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            children.append(child);
            parent->addChildObjectManyToMany(child);
            Qp::update(child);
        }

        parents.append(parent);
    }

    Tree tree;
    tree.children = children;
    tree.parents = parents;
    testTree(tree);
}

void ManyToManyRelationsTest::testDatabaseFKChangeFromParent()
{
    // Add a new child
    {
        Tree tree = createTree();
        QSharedPointer<ParentObject> addedToParent = tree.parents.first();
        QSharedPointer<ChildObject> addedChild = Qp::create<ChildObject>();
        addedToParent->addChildObjectManyToMany(addedChild);
        Qp::update(addedToParent);
        testTree(tree);
    }

    // Remove a child
    {
        Tree tree = createTree();
        QSharedPointer<ParentObject> removedFromParent = tree.parents.at(1);
        QSharedPointer<ChildObject> removedChild = Qp::create<ChildObject>();
        removedFromParent->removeChildObjectManyToMany(removedChild);
        Qp::update(removedFromParent);
        testTree(tree);
    }

    // Add one child from one parent to another and update the added-to parent
    {
        Tree tree = createTree();
        QSharedPointer<ParentObject> addedFromParent = tree.parents.first();
        QSharedPointer<ParentObject> addedToParent = tree.parents.at(1);
        QSharedPointer<ChildObject> changedChild = addedFromParent->childObjectsManyToMany().first();
        addedToParent->addChildObjectManyToMany(changedChild);
        Qp::update(addedToParent);

        testTree(tree);
    }
}

void ManyToManyRelationsTest::testDatabaseFKChangeFromChild()
{
    // Add a new child
    {
        Tree tree = createTree();

        QSharedPointer<ParentObject> addedToParent = tree.parents.first();
        QSharedPointer<ChildObject> addedChild = Qp::create<ChildObject>();
        addedToParent->addChildObjectManyToMany(addedChild);
        Qp::update(addedChild);
        testTree(tree);
    }

    // Remove a child
    {
        Tree tree = createTree();
        QSharedPointer<ParentObject> removedFromParent = tree.parents.at(1);
        QSharedPointer<ChildObject> removedChild = Qp::create<ChildObject>();
        removedFromParent->removeChildObjectManyToMany(removedChild);
        Qp::update(removedChild);
        testTree(tree);
    }

    // Add one child from one parent to another
    {
        Tree tree = createTree();
        QSharedPointer<ParentObject> addedFromParent = tree.parents.first();
        QSharedPointer<ParentObject> addedToParent = tree.parents.at(1);
        QSharedPointer<ChildObject> changedChild = addedFromParent->childObjectsManyToMany().first();
        addedToParent->addChildObjectManyToMany(changedChild);
        Qp::update(changedChild);

        testTree(tree);
    }
}

void ManyToManyRelationsTest::testUpdateTimesFromParent()
{
    // Add a new child
    {
        Tree tree = createTree();
        Tree changedTree;

        QSharedPointer<ParentObject> addedToParent = tree.parents.first();
        QSharedPointer<ChildObject> addedChild = Qp::create<ChildObject>();
        QDateTime previousTime = Qp::updateTime(addedChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);

        addedToParent->addChildObjectManyToMany(addedChild);
        Qp::update(addedToParent);

        changedTree.children.append(addedChild);
        changedTree.parents.append(addedToParent);
        QDateTime newTime = Qp::updateTime(addedToParent);

        testUpdateTimes(previousTime, newTime, changedTree, tree);
    }

    // Remove a child
    {
        Tree tree = createTree();
        Tree changedTree;

        QSharedPointer<ParentObject> removedFromParent = tree.parents.at(1);
        QSharedPointer<ChildObject> removedChild = removedFromParent->childObjectsManyToMany().first();
        QDateTime previousTime = Qp::updateTime(removedChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);

        removedFromParent->removeChildObjectManyToMany(removedChild);
        Qp::update(removedFromParent);

        changedTree.children.append(removedChild);
        changedTree.parents.append(removedFromParent);
        QDateTime newTime = Qp::updateTime(removedFromParent);

        testUpdateTimes(previousTime, newTime, changedTree, tree);
    }

    // Add one child from one parent to another and update the added-to parent
    {
        Tree tree = createTree();
        Tree changedTree;

        QSharedPointer<ParentObject> addedFromParent = tree.parents.at(0);
        QSharedPointer<ParentObject> addedToParent = tree.parents.at(1);
        QSharedPointer<ChildObject> changedChild = addedFromParent->childObjectsManyToMany().first();
        QDateTime previousTime = Qp::updateTime(changedChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);

        addedToParent->addChildObjectManyToMany(changedChild);
        Qp::update(addedToParent);

        // The added-from parent does not change by this operation!
        changedTree.children.append(changedChild);
        changedTree.parents.append(addedToParent);

        QDateTime newTime = Qp::updateTime(addedToParent);

        testUpdateTimes(previousTime, newTime, changedTree, tree);
    }
}

void ManyToManyRelationsTest::testUpdateTimesFromChild()
{
    // Add a new child
    {
        Tree tree = createTree();
        Tree changedTree;

        QSharedPointer<ParentObject> addedToParent = tree.parents.first();
        QSharedPointer<ChildObject> addedChild = Qp::create<ChildObject>();
        QDateTime previousTime = Qp::updateTime(addedChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);

        addedToParent->addChildObjectManyToMany(addedChild);
        Qp::update(addedChild);

        changedTree.children.append(addedChild);
        changedTree.parents.append(addedToParent);
        QDateTime newTime = Qp::updateTime(addedChild);

        testUpdateTimes(previousTime, newTime, changedTree, tree);
    }

    // Remove a child
    {
        Tree tree = createTree();
        Tree changedTree;

        QSharedPointer<ParentObject> removedFromParent = tree.parents.at(1);
        QSharedPointer<ChildObject> removedChild = removedFromParent->childObjectsManyToMany().first();
        QDateTime previousTime = Qp::updateTime(removedChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);

        removedFromParent->removeChildObjectManyToMany(removedChild);
        Qp::update(removedChild);

        changedTree.children.append(removedChild);
        changedTree.parents.append(removedFromParent);
        QDateTime newTime = Qp::updateTime(removedChild);

        testUpdateTimes(previousTime, newTime, changedTree, tree);
    }

    // Add one child from one parent to another and update the added-to parent
    {
        Tree tree = createTree();
        Tree changedTree;

        QSharedPointer<ParentObject> addedFromParent = tree.parents.at(0);
        QSharedPointer<ParentObject> addedToParent = tree.parents.at(1);
        QSharedPointer<ChildObject> changedChild = addedFromParent->childObjectsManyToMany().first();
        QDateTime previousTime = Qp::updateTime(changedChild);

        qDebug() << "Sleeping 1 second...";
        QTest::qSleep(1000);

        addedToParent->addChildObjectManyToMany(changedChild);
        Qp::update(changedChild);

        // The added-from parent does not change by this operation!
        changedTree.children.append(changedChild);
        changedTree.parents.append(addedToParent);

        QDateTime newTime = Qp::updateTime(changedChild);

        testUpdateTimes(previousTime, newTime, changedTree, tree);
    }
}

QVariantList ManyToManyRelationsTest::childFKs(QSharedPointer<ParentObject> parent)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(m_childToParentRelation.columnName());
    select.setWhereCondition(QpSqlCondition(m_childToParentRelation.reverseRelation().columnName(),
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

QVariantList ManyToManyRelationsTest::parentFKs(QSharedPointer<ChildObject> child)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_parentToChildRelation.tableName());
    select.addField(m_parentToChildRelation.columnName());
    select.setWhereCondition(QpSqlCondition(m_parentToChildRelation.reverseRelation().columnName(),
                                            QpSqlCondition::EqualTo,
                                            Qp::primaryKey(child)));
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

void ManyToManyRelationsTest::testParentFks(QSharedPointer<ChildObject> child)
{
    QVariantList fks = parentFKs(child);

    foreach(QSharedPointer<ParentObject> parent, child->parentObjectsManyToMany()) {
        QVariant pk = Qp::primaryKey(parent);
        if(!fks.contains(pk))
            qDebug() << "###########";

        QVERIFY2(fks.contains(pk), QString("Missing FK: %1").arg(pk.toString()).toUtf8());
        fks.removeAll(pk);
    }

    QVERIFY(fks.isEmpty());
}

void ManyToManyRelationsTest::testChildFks(QSharedPointer<ParentObject> parent)
{
    QVariantList fks = childFKs(parent);

    foreach(QSharedPointer<ChildObject> child, parent->childObjectsManyToMany()) {
        QVariant pk = Qp::primaryKey(child);
        QVERIFY(fks.contains(pk));
        fks.removeAll(pk);
    }

    QVERIFY(fks.isEmpty());
}

void ManyToManyRelationsTest::testTree(ManyToManyRelationsTest::Tree tree)
{
    foreach(QSharedPointer<ParentObject> parent, tree.parents) {
        testChildFks(parent);
        foreach(QSharedPointer<ChildObject> child2, parent->childObjectsManyToMany()) {
            testParentFks(child2);
        }
    }
    foreach(QSharedPointer<ChildObject> child, tree.children) {
        testParentFks(child);
        foreach(QSharedPointer<ParentObject> parent, child->parentObjectsManyToMany()) {
            testChildFks(parent);
        }
    }
}

void ManyToManyRelationsTest::testUpdateTimes(QDateTime previousTime, QDateTime newTime,
                                              ManyToManyRelationsTest::Tree changed, ManyToManyRelationsTest::Tree completeTree)
{
    foreach(QSharedPointer<ParentObject> parent, completeTree.parents) {
        if(changed.parents.contains(parent)) {
            QCOMPARE(Qp::updateTime(parent), newTime);
        }
        else {
            QCOMPARE(Qp::updateTime(parent), previousTime);
        }

        foreach(QSharedPointer<ChildObject> child, parent->childObjectsManyToMany()) {
            if(changed.children.contains(child)) {
                QCOMPARE(Qp::updateTime(child), newTime);
            }
            else {
                QCOMPARE(Qp::updateTime(child), previousTime);
            }
        }
    }

    foreach(QSharedPointer<ChildObject> child, completeTree.children) {
        if(changed.children.contains(child)) {
            QCOMPARE(Qp::updateTime(child), newTime);
        }
        else {
            QCOMPARE(Qp::updateTime(child), previousTime);
        }

        foreach(QSharedPointer<ParentObject> parent, child->parentObjectsManyToMany()) {
            if(changed.parents.contains(parent)) {
                QCOMPARE(Qp::updateTime(parent), newTime);
            }
            else {
                QCOMPARE(Qp::updateTime(parent), previousTime);
            }
        }
    }
}

ManyToManyRelationsTest::Tree ManyToManyRelationsTest::createTree()
{
    QList<QSharedPointer<ParentObject>> parents;
    QList<QSharedPointer<ChildObject>> children;
    for(int j = 0; j < PARENTCOUNT; ++j) {
        QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

        for(int i = 0; i < CHILDCOUNT; ++i) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            children.append(child);
            parent->addChildObjectManyToMany(child);
        }

        parents.append(parent);
        Qp::update(parent);
    }

    Tree result;
    result.children = children;
    result.parents = parents;
    return result;
}
