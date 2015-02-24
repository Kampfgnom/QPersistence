#ifndef TST_ONETOMANYRELATIONTEST_H
#define TST_ONETOMANYRELATIONTEST_H

#include "tests_common.h"

class OneToManyRelationTest : public QObject
{
    Q_OBJECT
public:
    explicit OneToManyRelationTest(QObject *parent = 0);

private slots:
    void initTestCase();

    void testOneToManyRelation();
    void testInitialDatabaseFKEmpty();
    void testDatabaseFKInsertFromParent();
    void testDatabaseFKInsertFromChild();
    void testDatabaseFKChangeFromParent();
    void testDatabaseFKChangeFromChild();

private:
    QpMetaProperty m_parentToChildRelation;
    QpMetaProperty m_childToParentRelation;

    QVariantList childFKs(QSharedPointer<TestNameSpace::ParentObject> parent);
    QVariant parentFK(QSharedPointer<TestNameSpace::ChildObject> child);

    void testParentFk(QSharedPointer<TestNameSpace::ChildObject> child);
    void testChildFks(QSharedPointer<TestNameSpace::ParentObject> parent);
};

#endif // TST_ONETOMANYRELATIONTEST_H
