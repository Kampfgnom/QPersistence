#include "qpersistence.h"

#include "metaobject.h"
#include "dataaccessobject.h"
#include "databaseschema.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

namespace Qp {

void setDatabase(const QSqlDatabase &database)
{
    if(Qp::database().isOpen()) {
        Qp::database().close();
        QSqlDatabase::removeDatabase("Qp");
    }

    QSqlDatabase::cloneDatabase(database, "Qp");

    QpSqlQuery query(database);
    query.prepare("PRAGMA foreign_keys = 1;");
    if ( !query.exec()
         || query.lastError().isValid()) {
        qCritical() << "The PRAGMA foreign_keys could not be set to 1:" << query.lastError();
    }
}

QSqlDatabase database()
{
    return QSqlDatabase::database("Qp");
}

void adjustDatabaseSchema()
{
    QpDatabaseSchema schema(Qp::database());
    schema.adjustSchema();
}

void createCleanSchema()
{
    QpDatabaseSchema schema(Qp::database());
    schema.createCleanSchema();
}

}
