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
