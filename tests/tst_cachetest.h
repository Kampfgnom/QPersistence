#ifndef TST_CACHETEST_H
#define TST_CACHETEST_H

#include "tests_common.h"

class CacheTest : public QObject
{
    Q_OBJECT

public:
    CacheTest();

private slots:
    void testBasics();
    void testRemove();
    void testMaximumCacheSize();
    void testCacheReOrderingUponAccess();
};

#endif // TST_CACHETEST_H
