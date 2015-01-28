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
#include "tst_flagstest.h"
#include "tst_usermanagementtest.h"
#include "tst_propertydependenciestest.h"

#include "parentobject.h"
#include "childobject.h"

#include <QPersistence/legacysqldatasource.h>

#define RUNTEST(TestClass) { \
    TestClass t; \
    int ret = QTest::qExec(&t, argc, argv); \
    if(ret) \
    return ret; \
    }

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef QP_FOR_MYSQL
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("boot2docker");
    db.setDatabaseName("niklas");
    db.setUserName("niklas");
    db.setPassword("niklas");
#elif defined QP_FOR_SQLITE
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("testdb.sqlite");
#endif

    if(!db.open()) {
        qDebug() << db.lastError().text().toUtf8();
        return -1;
    }

    QpLegacySqlDatasource *ds = new QpLegacySqlDatasource(Qp::defaultStorage());
    ds->setSqlDatabase(db);
    Qp::defaultStorage()->setDatasource(ds);

#ifndef QP_NO_LOCKS
    foreach(QString field, LockTest::additionalLockInfo().keys()) {
        Qp::addAdditionalLockInformationField(field, LockTest::additionalLockInfo().value(field).type());
    }
    Qp::enableLocks();
#endif

    Qp::setDatabase(db);
    Qp::setSqlDebugEnabled(false);
    Qp::registerClass<TestNameSpace::ParentObject>();
    Qp::registerClass<TestNameSpace::ChildObject>();
    Qp::createCleanSchema();

    RUNTEST(PropertyDependenciesTest);
    RUNTEST(FlagsTest);
    RUNTEST(EnumerationTest);
    RUNTEST(CacheTest);
    RUNTEST(OneToOneRelationTest);
    RUNTEST(OneToManyRelationTest);
    RUNTEST(ManyToManyRelationsTest);

#ifndef QP_NO_TIMESTAMPS
    RUNTEST(CreationAndUpdateTimesTest);
    RUNTEST(SynchronizeTest);
#endif

#ifndef QP_NO_LOCKS
    RUNTEST(LockTest);
#endif

#ifndef QP_NO_USERMANAGEMENT
    RUNTEST(UserManagementTest);
#endif

    return 0;
}
