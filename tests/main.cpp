#include <QtTest>

#include "tst_cachetest.h"
#include "tst_metaobjecttest.h"
#include "tst_creationandupdatetimestest.h"
#include "tst_relationsindatabasetest.h"

int main(int argc, char *argv[])
{
//    {
//    CacheTest t;
//    int ret = QTest::qExec(&t, argc, argv);
//    if(ret)
//        return ret;
//    }

//    {
//    MetaObjectTest t;
//    int ret = QTest::qExec(&t, argc, argv);
//    if(ret)
//        return ret;
//    }

    {
    RelationsInDatabaseTest t;
    int ret = QTest::qExec(&t, argc, argv);
    if(ret)
        return ret;
    }

    {
    CreationAndUpdateTimesTest t;
    int ret = QTest::qExec(&t, argc, argv);
    if(ret)
        return ret;
    }

    return 0;
}

