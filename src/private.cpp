#include "private.h"

#include "databaseschema.h"
#include "metaproperty.h"
#include "sqldataaccessobjecthelper.h"
#include "qpersistence.h"

#include "error.h"

#include <QDateTime>
#include <QSharedPointer>

namespace Qp {

namespace Private {

int primaryKey(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY).toInt();
}

void setPrimaryKey(QObject *object, int key)
{
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,key);
}

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

typedef QHash<const QObject *, QWeakPointer<QObject>> WeakPointerHash;
Q_GLOBAL_STATIC(WeakPointerHash, WeakPointers)

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
    QpError::setLastError(error);
}

}

}
