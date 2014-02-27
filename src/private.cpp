#include "private.h"

#include "databaseschema.h"
#include "error.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqldataaccessobjecthelper.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDateTime>
#include <QSharedPointer>
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
