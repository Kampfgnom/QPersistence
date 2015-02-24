#include "tst_manytomanyrelationstest.h"

ManyToManyRelationsTest::ManyToManyRelationsTest(QObject *parent) :
    QObject(parent),
    CHILDCOUNT(3),
    PARENTCOUNT(2)
{
}

void ManyToManyRelationsTest::initTestCase()
{
    QpMetaObject metaObject = QpMetaObject::forClassName(TestNameSpace::ParentObject::staticMetaObject.className());
    m_parentToChildRelation = metaObject.metaProperty("childObjectsManyToMany");

    metaObject = QpMetaObject::forClassName(TestNameSpace::ChildObject::staticMetaObject.className());
    m_childToParentRelation = metaObject.metaProperty("parentObjectsManyToMany");
}

struct TestTree {
    QList<QSharedPointer<TestNameSpace::ParentObject>> p;
    QList<QSharedPointer<TestNameSpace::ChildObject>> c;
    QHash<QSharedPointer<TestNameSpace::ChildObject> , QList<QSharedPointer<TestNameSpace::ParentObject>>> parents;
    QHash<QSharedPointer<TestNameSpace::ParentObject>, QList<QSharedPointer<TestNameSpace::ChildObject>>> children;
};

void testTreeTest(TestTree tree);
void testTreeTest(TestTree tree) {
    for(auto it = tree.children.begin(); it != tree.children.end(); ++it) {
        QSharedPointer<TestNameSpace::ParentObject> parent = it.key();
        QList<QSharedPointer<TestNameSpace::ChildObject>> shouldHaveChildren = it.value();
        QList<QSharedPointer<TestNameSpace::ChildObject>> hasChildren = parent->hasManyMany();
        foreach(QSharedPointer<TestNameSpace::ChildObject> child, shouldHaveChildren) {
            QVERIFY(hasChildren.contains(child));
            hasChildren.removeOne(child);
        }
        QVERIFY(hasChildren.isEmpty());
    }
    for(auto it = tree.parents.begin(); it != tree.parents.end(); ++it) {
        QSharedPointer<TestNameSpace::ChildObject> child = it.key();
        QList<QSharedPointer<TestNameSpace::ParentObject>> shouldHaveParents = it.value();
        QList<QSharedPointer<TestNameSpace::ParentObject>> hasParents = child->belongsToManyMany();
        foreach(QSharedPointer<TestNameSpace::ParentObject> parent, shouldHaveParents) {
            QVERIFY(hasParents.contains(parent));
            hasParents.removeOne(parent);
        }
        QVERIFY(hasParents.isEmpty());
    }
}

TestTree createTree();
TestTree createTree() {
    TestTree tree;

    // Create some objects
    for(int j = 0; j < 7; ++j) {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
        parent->setObjectName(QString("P%1").arg(j));
        tree.p.append(parent);

        for(int i = 0; i < 7; ++i) {
            QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
            child->setObjectName(QString("P%1_C%2").arg(j).arg(i));
            tree.children[parent].append(child);
            tree.parents[child].append(parent);
            parent->addHasManyMany(child);
            tree.c.append(child);
        }
    }

    for(int i = 0; i < tree.p.size(); ++i) {
        for(int j = 0; j < tree.c.size(); ++j) {
            QSharedPointer<TestNameSpace::ParentObject> parent = tree.p.at(i);
            QSharedPointer<TestNameSpace::ChildObject> child = tree.c.at(j);
            child->addBelongsToManyMany(parent);
            tree.children[parent].removeAll(child);
            tree.children[parent].append(child);
            tree.parents[child].removeAll(parent);
            tree.parents[child].append(parent);
        }
    }

    return tree;
}

void ManyToManyRelationsTest::testManyToManyRelation()
{
    {
        TestTree tree = ::createTree();
        testTreeTest(tree);
    }

    {
        TestTree tree = ::createTree();
        // Remove some children from parents
        for(auto it = tree.children.begin(); it != tree.children.end(); ++it) {
            QSharedPointer<TestNameSpace::ParentObject> parent = it.key();
            QList<QSharedPointer<TestNameSpace::ChildObject>> hasChildren = parent->hasManyMany();

            {
                QSharedPointer<TestNameSpace::ChildObject> removed = hasChildren.at(1);
                parent->removeHasManyMany(removed);

                tree.children[parent].removeAll(removed);
                tree.parents[removed].removeAll(parent);
            }
            {
                QSharedPointer<TestNameSpace::ChildObject> removed = hasChildren.at(1);
                parent->removeHasManyMany(removed);

                tree.children[parent].removeAll(removed);
                tree.parents[removed].removeAll(parent);
            }
        }
        testTreeTest(tree);
    }

    {
        TestTree tree = ::createTree();
        // Remove some parents from children
        for(auto it = tree.parents.begin(); it != tree.parents.end(); ++it) {
            QSharedPointer<TestNameSpace::ChildObject> child = it.key();
            QList<QSharedPointer<TestNameSpace::ParentObject>> hasParents = child->belongsToManyMany();

            {
                QSharedPointer<TestNameSpace::ParentObject> removed = hasParents.at(1);
                child->removeBelongsToManyMany(removed);

                tree.children[removed].removeAll(child);
                tree.parents[child].removeAll(removed);
            }
            {
                QSharedPointer<TestNameSpace::ParentObject> removed = hasParents.at(1);
                child->removeBelongsToManyMany(removed);

                tree.children[removed].removeAll(child);
                tree.parents[child].removeAll(removed);
            }
        }
        testTreeTest(tree);
    }
}

void ManyToManyRelationsTest::testInitialDatabaseFKsEmpty()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
    QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();

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
    QList<QSharedPointer<TestNameSpace::ParentObject>> parents;
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int j = 0; j < PARENTCOUNT; ++j) {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();

        for(int i = 0; i < CHILDCOUNT; ++i) {
            QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
            children.append(child);
            parent->addChildObjectsManyToMany(child);
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
        QSharedPointer<TestNameSpace::ParentObject> addedToParent = tree.parents.first();
        QSharedPointer<TestNameSpace::ChildObject> addedChild = Qp::create<TestNameSpace::ChildObject>();
        addedToParent->addChildObjectsManyToMany(addedChild);
        Qp::update(addedToParent);
        testTree(tree);
    }

    // Remove a child
    {
        Tree tree = createTree();
        QSharedPointer<TestNameSpace::ParentObject> removedFromParent = tree.parents.at(1);
        QSharedPointer<TestNameSpace::ChildObject> removedChild = Qp::create<TestNameSpace::ChildObject>();
        removedFromParent->removeChildObjectsManyToMany(removedChild);
        Qp::update(removedFromParent);
        testTree(tree);
    }

    // Add one child from one parent to another and update the added-to parent
    {
        Tree tree = createTree();
        QSharedPointer<TestNameSpace::ParentObject> addedFromParent = tree.parents.first();
        QSharedPointer<TestNameSpace::ParentObject> addedToParent = tree.parents.at(1);
        QSharedPointer<TestNameSpace::ChildObject> changedChild = addedFromParent->childObjectsManyToMany().first();
        addedToParent->addChildObjectsManyToMany(changedChild);
        Qp::update(addedToParent);

        testTree(tree);
    }
}

void ManyToManyRelationsTest::testDatabaseFKChangeFromChild()
{
    // Add a new child
    {
        Tree tree = createTree();

        QSharedPointer<TestNameSpace::ParentObject> addedToParent = tree.parents.first();
        QSharedPointer<TestNameSpace::ChildObject> addedChild = Qp::create<TestNameSpace::ChildObject>();
        addedToParent->addChildObjectsManyToMany(addedChild);
        Qp::update(addedChild);
        testTree(tree);
    }

    // Remove a child
    {
        Tree tree = createTree();
        QSharedPointer<TestNameSpace::ParentObject> removedFromParent = tree.parents.at(1);
        QSharedPointer<TestNameSpace::ChildObject> removedChild = Qp::create<TestNameSpace::ChildObject>();
        removedFromParent->removeChildObjectsManyToMany(removedChild);
        Qp::update(removedChild);
        testTree(tree);
    }

    // Add one child from one parent to another
    {
        Tree tree = createTree();
        QSharedPointer<TestNameSpace::ParentObject> addedFromParent = tree.parents.first();
        QSharedPointer<TestNameSpace::ParentObject> addedToParent = tree.parents.at(1);
        QSharedPointer<TestNameSpace::ChildObject> changedChild = addedFromParent->childObjectsManyToMany().first();
        Qp::synchronize(changedChild);
        addedToParent->addChildObjectsManyToMany(changedChild);
        Qp::update(changedChild);

        testTree(tree);
    }
}

QVariantList ManyToManyRelationsTest::childFKs(QSharedPointer<TestNameSpace::ParentObject> parent)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_childToParentRelation.tableName());
    select.addField(m_childToParentRelation.columnName());
    select.setWhereCondition(QpCondition(m_childToParentRelation.reverseRelation().columnName(),
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

QVariantList ManyToManyRelationsTest::parentFKs(QSharedPointer<TestNameSpace::ChildObject> child)
{
    QpSqlQuery select(Qp::database());
    select.setTable(m_parentToChildRelation.tableName());
    select.addField(m_parentToChildRelation.columnName());
    select.setWhereCondition(QpCondition(m_parentToChildRelation.reverseRelation().columnName(),
                                            QpCondition::EqualTo,
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

void ManyToManyRelationsTest::testParentFks(QSharedPointer<TestNameSpace::ChildObject> child)
{
    QVariantList fks = parentFKs(child);

    foreach(QSharedPointer<TestNameSpace::ParentObject> parent, child->parentObjectsManyToMany()) {
        QVariant pk = Qp::primaryKey(parent);
        if(!fks.contains(pk))
            qDebug() << "###########";

        QVERIFY2(fks.contains(pk), QString("Missing FK: %1").arg(pk.toString()).toUtf8());
        fks.removeAll(pk);
    }

    QVERIFY(fks.isEmpty());
}

void ManyToManyRelationsTest::testChildFks(QSharedPointer<TestNameSpace::ParentObject> parent)
{
    QVariantList fks = childFKs(parent);

    foreach(QSharedPointer<TestNameSpace::ChildObject> child, parent->childObjectsManyToMany()) {
        QVariant pk = Qp::primaryKey(child);
        QVERIFY(fks.contains(pk));
        fks.removeAll(pk);
    }

    QVERIFY(fks.isEmpty());
}

void ManyToManyRelationsTest::testTree(ManyToManyRelationsTest::Tree tree)
{
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wshadow\"");
    foreach(QSharedPointer<TestNameSpace::ParentObject> parent, tree.parents) {
        testChildFks(parent);
        foreach(QSharedPointer<TestNameSpace::ChildObject> child2, parent->childObjectsManyToMany()) {
            testParentFks(child2);
        }
    }
    foreach(QSharedPointer<TestNameSpace::ChildObject> child, tree.children) {
        testParentFks(child);
        foreach(QSharedPointer<TestNameSpace::ParentObject> parent, child->parentObjectsManyToMany()) {
            testChildFks(parent);
        }
    }
    _Pragma("GCC diagnostic pop");
}

ManyToManyRelationsTest::Tree ManyToManyRelationsTest::createTree()
{
    QList<QSharedPointer<TestNameSpace::ParentObject>> parents;
    QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    for(int j = 0; j < PARENTCOUNT; ++j) {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();

        for(int i = 0; i < CHILDCOUNT; ++i) {
            QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
            children.append(child);
            parent->addChildObjectsManyToMany(child);
        }

        parents.append(parent);
        Qp::update(parent);
    }

    Tree result;
    result.children = children;
    result.parents = parents;
    return result;
}
