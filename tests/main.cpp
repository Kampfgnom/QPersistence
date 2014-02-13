#include <QtTest>

#include "tst_cachetest.h"
#include "tst_onetoonerelationtest.h"
#include "tst_onetomanyrelationtest.h"
#include "tst_manytomanyrelationstest.h"
#include "tst_creationandupdatetimestest.h"
#include "tst_synchronizetest.h"
#include "tst_locktest.h"

#define RUNTEST(TestClass) { \
    TestClass t; \
    int ret = QTest::qExec(&t, argc, argv); \
    if(ret) \
    return ret; \
    }

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//    RUNTEST(CacheTest);
//    RUNTEST(OneToOneRelationTest);
//    RUNTEST(OneToManyRelationTest);
//    RUNTEST(ManyToManyRelationsTest);
//    RUNTEST(CreationAndUpdateTimesTest);
//    RUNTEST(SynchronizeTest);
    RUNTEST(LockTest);

    return 0;
}
