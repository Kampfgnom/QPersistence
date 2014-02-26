
#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtTest>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "tst_cachetest.h"
#include "tst_onetoonerelationtest.h"
#include "tst_onetomanyrelationtest.h"
#include "tst_manytomanyrelationstest.h"
#include "tst_creationandupdatetimestest.h"
#include "tst_synchronizetest.h"
#include "tst_locktest.h"
#include "tst_enumerationtest.h"

#define RUNTEST(TestClass) { \
    TestClass t; \
    int ret = QTest::qExec(&t, argc, argv); \
    if(ret) \
    return ret; \
    }

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    RUNTEST(LockTest);
    RUNTEST(EnumerationTest);
    RUNTEST(CacheTest);
    RUNTEST(OneToOneRelationTest);
    RUNTEST(OneToManyRelationTest);
    RUNTEST(ManyToManyRelationsTest);
    RUNTEST(CreationAndUpdateTimesTest);
    RUNTEST(SynchronizeTest);

    return 0;
}
