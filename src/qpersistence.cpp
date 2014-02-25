#include "qpersistence.h"

#include "dataaccessobject.h"
#include "databaseschema.h"
#include "error.h"
#include "lock.h"
#include "metaobject.h"
#include "sqlbackend.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

namespace Qp {

void setDatabase(const QSqlDatabase &database)
{
    if (Qp::database().isOpen()) {
        Qp::database().close();
        QSqlDatabase::removeDatabase("Qp");
    }

    QSqlDatabase::cloneDatabase(database, "Qp");

    if(database.driverName() == QLatin1String("QSQLITE")) {
        QpSqlQuery query(database);
        query.prepare("PRAGMA foreign_keys = 1;");
        if (!query.exec()
                || query.lastError().isValid()) {
            qCritical() << "The PRAGMA foreign_keys could not be set to 1:" << query.lastError();
        }
    }
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
    bool transaction = Qp::database().transaction();
    if(!transaction)
        qFatal("START TRANSACTION failed.");

    if(QpSqlQuery::isDebugEnabled())
        qDebug() << "START TRANSACTION;";

    return transaction;
}

CommitResult commitOrRollbackTransaction()
{
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
    if(!commit)
        qFatal("COMMIT failed.");

    if(QpSqlQuery::isDebugEnabled())
        qDebug() << "COMMIT;";
    if(commit)
        return CommitSuccessful;
    else
        return CommitFailed;
}

void enableLocks()
{
    QpLock::enableLocks();
}

void addAdditionalLockInformationField(const QString &field, QVariant::Type type)
{
    QpLock::addAdditionalInformationField(field, type);
}

QDateTime databaseTime()
{
    QpSqlQuery query(Qp::database());
    if(!query.exec(QString("SELECT %1").arg(QpSqlBackend::forDatabase(Qp::database())->nowTimestamp()))
            || !query.first()) {
        Qp::Private::setLastError(QpError(query.lastError()));
        return QDateTime();
    }

    return dateFromDouble(query.value(0).toDouble());
}

QDateTime dateFromDouble(double value)
{
    QString string = QString("%1").arg(value, 17, 'f', 3);
    return QDateTime::fromString(string, "yyyyMMddHHmmss.zzz");
}

}
