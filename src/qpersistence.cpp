#include "qpersistence.h"

#include "dataaccessobject.h"
#include "databaseschema.h"
#include "error.h"
#include "lock.h"
#include "metaobject.h"
#include "sqlbackend.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDebug>
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

namespace Qp {

void setDatabase(const QSqlDatabase &database)
{
    if (Qp::database().isOpen()) {
        Qp::database().close();
        QSqlDatabase::removeDatabase("Qp");
    }

    QSqlDatabase::cloneDatabase(database, "Qp");

#ifdef QP_FOR_SQLITE
    QpSqlQuery query(database);
    query.prepare("PRAGMA foreign_keys = 1;");
    if (!query.exec()
            || query.lastError().isValid()) {
        qCritical() << "The PRAGMA foreign_keys could not be set to 1:" << query.lastError();
    }
#endif
}

QSqlDatabase database()
{
    return QSqlDatabase::database("Qp");
}

bool adjustDatabaseSchema()
{
    beginTransaction();
    QpDatabaseSchema schema(Qp::database());
    schema.adjustSchema();
    return commitOrRollbackTransaction() == CommitSuccessful;
}

bool createCleanSchema()
{
    beginTransaction();
    QpDatabaseSchema schema(Qp::database());
    schema.createCleanSchema();
    return commitOrRollbackTransaction() == CommitSuccessful;
}

void startBulkDatabaseQueries()
{
    QpSqlQuery::startBulkExec();
}

void commitBulkDatabaseQueries()
{
    QpSqlQuery::bulkExec();
}

void setSqlDebugEnabled(bool enable)
{
    QpSqlQuery::setDebugEnabled(enable);
}

QpError lastError()
{
    if(QpError::lastError().isValid())
        return QpError::lastError();

    if(Qp::database().lastError().isValid())
        return QpError(Qp::database().lastError());

    return QpError();
}

bool beginTransaction()
{
    if(Qp::database().driverName() != "QMYSQL")
        return true;

    bool transaction = Qp::database().transaction();
    if(!transaction)
        qFatal("START TRANSACTION failed.");

    if(QpSqlQuery::isDebugEnabled())
        qDebug() << "START TRANSACTION;";

    return transaction;
}

CommitResult commitOrRollbackTransaction()
{
    if(Qp::database().driverName() != "QMYSQL")
        return CommitSuccessful;

    if(lastError().isValid()) {
        bool rollback = Qp::database().rollback();
        if(!rollback)
            qFatal("ROLLBACK failed.");

        if(QpSqlQuery::isDebugEnabled())
            qDebug() << "ROLLBACK;";
        if(rollback)
            return RollbackSuccessful;
        else
            return RollbackFailed;
    }

    bool commit = Qp::database().commit();
    if(!commit) {
        qWarning() << Qp::database().lastError();
        qFatal("COMMIT failed.");
    }

    if(QpSqlQuery::isDebugEnabled())
        qDebug() << "COMMIT;";
    if(commit)
        return CommitSuccessful;
    else
        return CommitFailed;
}

#ifndef QP_NO_LOCKS
void enableLocks()
{
    QpLock::enableLocks();
}

void addAdditionalLockInformationField(const QString &field, QVariant::Type type)
{
    QpLock::addAdditionalInformationField(field, type);
}
#endif

#ifndef QP_NO_TIMESTAMPS
QDateTime dateFromDouble(double value)
{
    QString string = QString("%1").arg(value, 17, 'f', 3);
    return QDateTime::fromString(string, "yyyyMMddHHmmss.zzz");
}

QDateTime databaseTime()
{
    return dateFromDouble(Private::databaseTime());
}
#endif

}
