#ifndef ONETOONERELATIONTEST_H
#define ONETOONERELATIONTEST_H

#include "tests_common.h"

class OneToOneRelationTest : public QObject
{
    Q_OBJECT
public:
    explicit OneToOneRelationTest(QObject *parent = 0);

private slots:
    void initTestCase();

    void testOneToOneRelation();
    void testInitialDatabaseFKEmpty();
    void testDatabaseFKInsertFromParent();
    void testDatabaseFKInsertFromChild();
    void testDatabaseFKChangeFromParent();
    void testDatabaseFKChangeFromChild();
    void testDatabaseFKClearFromParent();
    void testDatabaseFKClearFromChild();

#ifndef QP_NO_TIMESTAMPS
    void testDatabaseUpdateTimes();
#endif

private:
    QpMetaProperty m_parentToChildRelation;
    QpMetaProperty m_childToParentRelation;

    QVariant childFK(QSharedPointer<TestNameSpace::ParentObject> parent);
    QVariant parentFK(QSharedPointer<TestNameSpace::ChildObject> child);
};

#endif // ONETOONERELATIONTEST_H
