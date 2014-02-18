#ifndef ONETOONERELATIONTEST_H
#define ONETOONERELATIONTEST_H

#include "relationtestbase.h"

class OneToOneRelationTest : public RelationTestBase
{
    Q_OBJECT
public:
    explicit OneToOneRelationTest(QObject *parent = 0);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testOneToOneRelation();

private:
    void testInitialDatabaseFKEmpty();

    void testDatabaseFKInsertFromParent();
    void testDatabaseFKInsertFromChild();

    void testDatabaseFKChangeFromParent();
    void testDatabaseFKChangeFromChild();

    void testDatabaseFKClearFromParent();
    void testDatabaseFKClearFromChild();

    void testDatabaseUpdateTimes();

private:
    QpMetaProperty m_parentToChildRelation;
    QpMetaProperty m_childToParentRelation;

    QVariant childFK(QSharedPointer<ParentObject> parent);
    QVariant parentFK(QSharedPointer<ChildObject> child);
};

#endif // ONETOONERELATIONTEST_H
