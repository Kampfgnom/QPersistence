#ifndef TST_ONETOMANYRELATIONTEST_H
#define TST_ONETOMANYRELATIONTEST_H

#include "relationtestbase.h"

class OneToManyRelationTest : public RelationTestBase
{
    Q_OBJECT
public:
    explicit OneToManyRelationTest(QObject *parent = 0);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testInitialDatabaseFKEmpty();

    void testDatabaseFKInsertFromParent();
    void testDatabaseFKInsertFromChild();

    void testDatabaseFKChangeFromParent();
    void testDatabaseFKChangeFromChild();

    void testUpdateTimesFromParent();
    void testUpdateTimesFromChild();

private:
    QpMetaProperty m_parentToChildRelation;
    QpMetaProperty m_childToParentRelation;

    QVariantList childFKs(QSharedPointer<ParentObject> parent);
    QVariant parentFK(QSharedPointer<ChildObject> child);

    void testParentFk(QSharedPointer<ChildObject> child);
    void testChildFks(QSharedPointer<ParentObject> parent);
};

#endif // TST_ONETOMANYRELATIONTEST_H
