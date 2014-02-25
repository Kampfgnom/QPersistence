#ifndef TST_CACHETEST_H
#define TST_CACHETEST_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QString>
#include <QtTest>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class CacheTest : public QObject
{
    Q_OBJECT

public:
    CacheTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testBasics();
    void testRemove();
    void testMaximumCacheSize();
    void testCacheReOrderingUponAccess();
};

#endif // TST_CACHETEST_H
