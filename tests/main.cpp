#include <QtTest>

#include "tst_cachetest.h"
#include "tst_metaobjecttest.h"

int main(int argc, char *argv[])
{
    CacheTest tc;
    QTest::qExec(&tc, argc, argv);
    MetaObjectTest mot;
    QTest::qExec(&mot, argc, argv);

    return 0;
}

