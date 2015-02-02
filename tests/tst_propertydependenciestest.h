#ifndef PROPERTYDEPENDENCIESTEST_H
#define PROPERTYDEPENDENCIESTEST_H

#include "tests_common.h"

class PropertyDependenciesTest : public QObject
{
    Q_OBJECT
public:
    explicit PropertyDependenciesTest(QObject *parent = 0);
    ~PropertyDependenciesTest();

private slots:
    void initTestCase();

    void testSelfDependency();
    void testHasOneDependency();
    void testHasManyDependency();
    void testHasManyManyDependency();

    void testBelongsToOneDependency();
    void testBelongsToManyDependency();
    void testBelongsManyToManyDependency();
};

#endif // PROPERTYDEPENDENCIESTEST_H
