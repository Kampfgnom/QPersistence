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
#include <QDebug>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

namespace Qp {

namespace Private {

int primaryKey(const QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY).toInt();
}

int primaryKey(QSharedPointer<QObject> object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY).toInt();
}

QList<int> primaryKeys(const QList<QSharedPointer<QObject> > &objects)
{
    QList<int> result;
    foreach(QSharedPointer<QObject> object, objects) {
        result << primaryKey(object.data());
    }
    return result;
}

void setPrimaryKey(QObject *object, int key)
{
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,key);
}

int revisionInObject(const QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_REVISION).toInt();
}

bool isDeleted(QSharedPointer<QObject> object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG).toBool();
}

bool isDeleted(const QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG).toBool();
}

void markAsDeleted(QObject *object)
{
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG,true);
}

void undelete(QObject *object)
{
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG,false);
}

typedef QHash<const QObject *, QWeakPointer<QObject> > WeakPointerHash;
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

}

}
