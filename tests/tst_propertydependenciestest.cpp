#include "tst_propertydependenciestest.h"

#include <parentobject.h>
#include "../src/cache.h"

PropertyDependenciesTest::PropertyDependenciesTest(QObject *parent) : QObject(parent)
{

}

PropertyDependenciesTest::~PropertyDependenciesTest()
{

}

void PropertyDependenciesTest::initTestCase()
{
    Qp::dataAccessObject<TestNameSpace::ParentObject>()->cache().setMaximumCacheSize(0);
    Qp::dataAccessObject<TestNameSpace::ChildObject>()->cache().setMaximumCacheSize(0);
}

void PropertyDependenciesTest::testSelfDependency()
{
    QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();

    QCOMPARE(parent->calculatedIntDependency(), 0);
    QCOMPARE(parent->calculatedInt(), 0);
    QCOMPARE(parent->calculatedInt2(), 0);

    int value = 35;
    parent->setCalculatedIntDependency(value);
    QCOMPARE(parent->calculatedIntDependency(), value);
    QCOMPARE(parent->calculatedInt(), value * 5);
    QCOMPARE(parent->calculatedInt2(), value * 5 * 23);
}

void PropertyDependenciesTest::testHasOneDependency()
{
    int parentId = 0;
    int v1 = 7;
    int v2 = 11;
    {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
        QCOMPARE(parent->calculatedFromHasOne(), 0);
        parentId = Qp::primaryKey(parent);

        QSharedPointer<TestNameSpace::ChildObject> child1 = Qp::create<TestNameSpace::ChildObject>();
        child1->setCalculatedIntDependency(v1);

        parent->setHasOne(child1);
        QCOMPARE(parent->calculatedFromHasOne(), child1->calculatedIntDependency() * 2);

        child1->setCalculatedIntDependency(v2);
        QCOMPARE(parent->calculatedFromHasOne(), child1->calculatedIntDependency() * 2);

        Qp::update(child1);
        parent.clear();
    }

    {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::read<TestNameSpace::ParentObject>(parentId);
        QCOMPARE(parent->calculatedFromHasOne(), v2 * 2);
        QSharedPointer<TestNameSpace::ChildObject> child1 = parent->hasOne();

        parent->setHasOne(QSharedPointer<TestNameSpace::ChildObject>());
        QCOMPARE(parent->calculatedFromHasOne(), 0);

        child1->setCalculatedIntDependency(12);
        QCOMPARE(parent->calculatedFromHasOne(), 0);
    }
}

void PropertyDependenciesTest::testHasManyDependency()
{
    int parentId = 0;
    int childId = 0;
    int factor = 3;
    int v1 = 7;
    int v2 = 11;
    int v3 = 13;
    {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
        QCOMPARE(parent->calculatedFromHasMany(), 0);
        parentId = Qp::primaryKey(parent);

        QSharedPointer<TestNameSpace::ChildObject> child1 = Qp::create<TestNameSpace::ChildObject>();
        child1->setCalculatedIntDependency(v1);
        childId = Qp::primaryKey(child1);

        parent->addHasMany(child1);
        QCOMPARE(parent->calculatedFromHasMany(), child1->calculatedIntDependency() * factor);

        QSharedPointer<TestNameSpace::ChildObject> child2 = Qp::create<TestNameSpace::ChildObject>();
        child2->setCalculatedIntDependency(v2);

        parent->addHasMany(child2);
        QCOMPARE(parent->calculatedFromHasMany(),  v1 * factor + v2 * factor);

        child1->setCalculatedIntDependency(v3);
        QCOMPARE(parent->calculatedFromHasMany(),  v3 *factor + v2 * factor);

        Qp::update(child1);
        Qp::update(child2);
        parent.clear();
    }

    {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::read<TestNameSpace::ParentObject>(parentId);
        QCOMPARE(parent->calculatedFromHasMany(), v2 * factor + v3 * factor);

        QSharedPointer<TestNameSpace::ChildObject> child1 = Qp::read<TestNameSpace::ChildObject>(childId);
        parent->removeHasMany(child1);
        QCOMPARE(parent->calculatedFromHasMany(), v2 * factor);

        child1->setCalculatedIntDependency(17);
        QCOMPARE(parent->calculatedFromHasMany(), v2 * factor);
    }
}

void PropertyDependenciesTest::testHasManyManyDependency()
{
    int parentId = 0;
    int childId = 0;
    int factor = 5;
    int v1 = 7;
    int v2 = 11;
    int v3 = 13;
    {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
        QCOMPARE(parent->calculatedFromHasManyMany(), 0);
        parentId = Qp::primaryKey(parent);

        QSharedPointer<TestNameSpace::ChildObject> child1 = Qp::create<TestNameSpace::ChildObject>();
        child1->setCalculatedIntDependency(v1);
        childId = Qp::primaryKey(child1);

        parent->addHasManyMany(child1);
        QCOMPARE(parent->calculatedFromHasManyMany(), child1->calculatedIntDependency() * factor);

        QSharedPointer<TestNameSpace::ChildObject> child2 = Qp::create<TestNameSpace::ChildObject>();
        child2->setCalculatedIntDependency(v2);

        parent->addHasManyMany(child2);
        QCOMPARE(parent->calculatedFromHasManyMany(),  v1 * factor + v2 * factor);

        child1->setCalculatedIntDependency(v3);
        QCOMPARE(parent->calculatedFromHasManyMany(),  v3 * factor + v2 * factor);

        Qp::update(child1);
        Qp::update(child2);
        parent.clear();
    }

    {
        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::read<TestNameSpace::ParentObject>(parentId);
        QCOMPARE(parent->calculatedFromHasManyMany(), v2 * factor + v3 * factor);

        QSharedPointer<TestNameSpace::ChildObject> child1 = Qp::read<TestNameSpace::ChildObject>(childId);
        parent->removeHasManyMany(child1);
        QCOMPARE(parent->calculatedFromHasManyMany(), v2 * factor);

        child1->setCalculatedIntDependency(17);
        QCOMPARE(parent->calculatedFromHasManyMany(), v2 * factor);
    }
}

void PropertyDependenciesTest::testBelongsToOneDependency()
{
    int childId = 0;
    int v1 = 7;
    int v2 = 11;
    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        QCOMPARE(child->calculatedFromToOne(), 0);
        childId = Qp::primaryKey(child);

        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
        parent->setCalculatedIntDependency(v1);

        parent->setHasOne(child);
        QCOMPARE(child->calculatedFromToOne(), v1 * 17);

        parent->setCalculatedIntDependency(v2);
        QCOMPARE(child->calculatedFromToOne(), v2 * 17);

        Qp::update(parent);
    }

    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::read<TestNameSpace::ChildObject>(childId);
        QCOMPARE(child->calculatedFromToOne(), v2 * 17);

        QSharedPointer<TestNameSpace::ParentObject> parent = child->belongsToOne();
        parent->setHasOne(QSharedPointer<TestNameSpace::ChildObject>());
        QCOMPARE(child->calculatedFromToOne(), 0);

        parent->setCalculatedIntDependency(55);
        QCOMPARE(child->calculatedFromToOne(), 0);
    }
}

void PropertyDependenciesTest::testBelongsToManyDependency()
{
    int childId = 0;
    int v1 = 7;
    int v2 = 11;
    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        QCOMPARE(child->calculatedFromToMany(), 0);
        childId = Qp::primaryKey(child);

        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
        parent->setCalculatedIntDependency(v1);

        parent->addHasMany(child);
        QCOMPARE(child->calculatedFromToMany(), v1 * 23);

        QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();
        parent2->addHasMany(child);
        parent2->setCalculatedIntDependency(v2);
        QCOMPARE(child->calculatedFromToMany(), v2 * 23);

        Qp::update(parent2);
    }

    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::read<TestNameSpace::ChildObject>(childId);
        QCOMPARE(child->calculatedFromToMany(), v2 * 23);

        QSharedPointer<TestNameSpace::ParentObject> parent = child->belongsToOneMany();
        parent->removeHasMany(child);
        QCOMPARE(child->calculatedFromToMany(), 0);

        parent->setCalculatedIntDependency(55);
        QCOMPARE(child->calculatedFromToMany(), 0);
    }
}

void PropertyDependenciesTest::testBelongsManyToManyDependency()
{
    int childId = 0;
    int parentId = 0;
    int v1 = 7;
    int v2 = 11;
    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
        QCOMPARE(child->calculatedFromManyToMany(), 0);
        childId = Qp::primaryKey(child);

        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::create<TestNameSpace::ParentObject>();
        parent->setCalculatedIntDependency(v1);
        parentId = Qp::primaryKey(parent);

        parent->addHasManyMany(child);
        QCOMPARE(child->calculatedFromManyToMany(), v1 * 29);

        QSharedPointer<TestNameSpace::ParentObject> parent2 = Qp::create<TestNameSpace::ParentObject>();
        parent2->addHasManyMany(child);
        parent2->setCalculatedIntDependency(v2);
        QCOMPARE(child->calculatedFromManyToMany(), v1 * 29 + v2 * 29);

        Qp::update(parent);
        Qp::update(parent2);
    }

    {
        QSharedPointer<TestNameSpace::ChildObject> child = Qp::read<TestNameSpace::ChildObject>(childId);
        QCOMPARE(child->calculatedFromManyToMany(), v1 * 29 + v2 * 29);

        QSharedPointer<TestNameSpace::ParentObject> parent = Qp::read<TestNameSpace::ParentObject>(parentId);
        parent->removeHasManyMany(child);
        QCOMPARE(child->calculatedFromManyToMany(), v2 * 29);

        parent->setCalculatedIntDependency(55);
        QCOMPARE(child->calculatedFromManyToMany(), v2 * 29);
    }
}
