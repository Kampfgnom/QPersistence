#include "private.h"

#include "databaseschema.h"
#include "error.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqldataaccessobjecthelper.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDateTime>
#include <QSharedPointer>
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

namespace Qp {

namespace Private {

QP_DEFINE_STATIC_LOCAL(QObject, GlobalGuard)

int primaryKey(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY).toInt();
}

void setPrimaryKey(QObject *object, int key)
{
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,key);
}

bool isDeleted(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG).toBool();
}

void markAsDeleted(QObject *object)
{
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG,true);
}

#ifndef QP_NO_TIMESTAMPS
double creationTimeInDatabase(QObject *object)
{
    QpSqlDataAccessObjectHelper *daoHelper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    return daoHelper->readCreationTime(QpMetaObject::forObject(object), object);
}

double updateTimeInDatabase(QObject *object)
{
    QpSqlDataAccessObjectHelper *daoHelper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    return daoHelper->readUpdateTime(QpMetaObject::forObject(object), object);
}

double updateTimeInObject(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME).toDouble();
}

double creationTimeInObject(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME).toDouble();
}

double databaseTime()
{
    QpSqlQuery query(Qp::database());
    if(!query.exec(QString("SELECT %1").arg(QpSqlBackend::forDatabase(Qp::database())->nowTimestamp()))
            || !query.first()) {
        Qp::Private::setLastError(QpError(query.lastError()));
        return -1.0;
    }

    return query.value(0).toDouble();
}

#endif

typedef QHash<const QObject *, QWeakPointer<QObject>> WeakPointerHash;
QP_DEFINE_STATIC_LOCAL(WeakPointerHash, WeakPointers)

void enableSharedFromThis(QSharedPointer<QObject> object)
{
    WeakPointers()->insert(object.data(), object.toWeakRef());
}

QSharedPointer<QObject> sharedFrom(const QObject *object)
{
    QWeakPointer<QObject> weak = WeakPointers()->value(object);
    return weak.toStrongRef();
}

void setLastError(const QpError &error)
{
    qDebug() << error;
    QpError::setLastError(error);
}

}

}
