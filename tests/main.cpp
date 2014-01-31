#include <QtTest>

#include "tst_cachetest.h"
#include "tst_onetoonerelationtest.h"
#include "tst_onetomanyrelationtest.h"

int main(int argc, char *argv[])
{
    //    {
    //    CacheTest t;
    //    int ret = QTest::qExec(&t, argc, argv);
    //    if(ret)
    //        return ret;
    //    }

    //    {
    //    OneToOneRelationTest t;
    //    int ret = QTest::qExec(&t, argc, argv);
    //    if(ret)
    //        return ret;
    //    }

    {
        OneToManyRelationTest t;
        int ret = QTest::qExec(&t, argc, argv);
        if(ret)
            return ret;
    }

    return 0;
}
