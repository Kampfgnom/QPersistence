#ifndef TST_CACHETEST_H
#define TST_CACHETEST_H

#include <QString>
#include <QtTest>

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
    void setCacheOrder();
};



#endif // TST_CACHETEST_H
