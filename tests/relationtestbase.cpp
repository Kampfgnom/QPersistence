#include "relationtestbase.h"

#include <QSqlError>

RelationTestBase::RelationTestBase(QObject *parent) :
    QObject(parent)
{
}

void RelationTestBase::initDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("192.168.100.2");
    db.setDatabaseName("niklas");
    db.setUserName("niklas");
    db.setPassword("niklas");

    QVERIFY2(db.open(), db.lastError().text().toUtf8());

    Qp::setSqlDebugEnabled(true);
    Qp::setDatabase(db);
    Qp::registerClass<ParentObject>();
    Qp::registerClass<ChildObject>();
    Qp::createCleanSchema();

    VERIFY_QP_ERROR();
}

void RelationTestBase::VERIFY_QP_ERROR()
{
    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}
