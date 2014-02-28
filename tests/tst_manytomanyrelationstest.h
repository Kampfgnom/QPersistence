#ifndef TST_MANYTOMANYRELATIONSTEST_H
#define TST_MANYTOMANYRELATIONSTEST_H

#include "tests_common.h"

class ManyToManyRelationsTest : public QObject
{
    Q_OBJECT
public:
    explicit ManyToManyRelationsTest(QObject *parent = 0);

private Q_SLOTS:
    void initTestCase();

    void testManyToManyRelation();
    void testInitialDatabaseFKsEmpty();
    void testDatabaseFKInsertFromParent();
    void testDatabaseFKInsertFromChild();
    void testDatabaseFKChangeFromParent();
    void testDatabaseFKChangeFromChild();

#ifndef QP_NO_TIMESTAMPS
    void testUpdateTimesFromParent();
    void testUpdateTimesFromChild();
#endif

private:
    struct Tree {
        QList<QSharedPointer<TestNameSpace::ParentObject>> parents;
        QList<QSharedPointer<TestNameSpace::ChildObject>> children;
    };

    QpMetaProperty m_parentToChildRelation;
    QpMetaProperty m_childToParentRelation;

    QVariantList childFKs(QSharedPointer<TestNameSpace::ParentObject> parent);
    QVariantList parentFKs(QSharedPointer<TestNameSpace::ChildObject> child);

    void testParentFks(QSharedPointer<TestNameSpace::ChildObject> child);
    void testChildFks(QSharedPointer<TestNameSpace::ParentObject> parent);

    void testTree(Tree tree);

#ifndef QP_NO_TIMESTAMPS
    void testUpdateTimes(QDateTime previousTime, QDateTime newTime, Tree changed, Tree unchanged);
#endif

    Tree createTree();

    const int CHILDCOUNT = 3;
    const int PARENTCOUNT = 2;
};

#endif // TST_MANYTOMANYRELATIONSTEST_H
