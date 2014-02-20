#include "database.h"

#include <QSqlError>

RelationTestBase::RelationTestBase(QObject *parent) :
    QObject(parent)
{
}

void RelationTestBase::initDatabase()
{
    QSqlDatabase db = Qp::database();
    if(!db.isOpen()) {

#ifdef MYSQL
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("192.168.100.2");
        db.setDatabaseName("niklas");
        db.setUserName("niklas");
        db.setPassword("niklas");
#elif SQLITE
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("testdb.sqlite");
#endif

        QVERIFY2(db.open(), db.lastError().text().toUtf8());

        Qp::setDatabase(db);
        Qp::setSqlDebugEnabled(false);
        Qp::registerClass<ParentObject>();
        Qp::registerClass<ChildObject>();
        Qp::createCleanSchema();
    }
}

QVariant RelationTestBase::NULLKEY()
{
#ifdef MYSQL
    return RelationTestBase::NULLKEY();
#elif SQLITE
    return QVariant(QVariant().toString());
#endif
    return QVariant();
}

void RelationTestBase::VERIFY_QP_ERROR()
{
    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}
