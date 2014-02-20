#include "private.h"

#include "databaseschema.h"
#include "metaproperty.h"
#include "sqldataaccessobjecthelper.h"
#include "qpersistence.h"

#include "error.h"

#include <QDateTime>
#include <QDebug>
#include <QSharedPointer>

namespace Qp {

namespace Private {

int primaryKey(QObject *object)
{
    return object->property("_Qp_ID").toInt();
}

void setPrimaryKey(QObject *object, int key)
{
    object->setProperty("_Qp_ID",key);
}

#ifndef QP_LOCALDB
QDateTime creationTimeInDatabase(QObject *object)
{
    QpSqlDataAccessObjectHelper *daoHelper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    return daoHelper->readCreationTime(QpMetaObject::forClassName(object->metaObject()->className()), object);
}

QDateTime updateTimeInDatabase(QObject *object)
{
    QpSqlDataAccessObjectHelper *daoHelper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    return daoHelper->readUpdateTime(QpMetaObject::forClassName(object->metaObject()->className()), object);
}

QDateTime updateTimeInObject(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME.toLatin1()).toDateTime();
}
#endif

void enableSharedFromThis(QSharedPointer<QObject> object)
{
    QWeakPointer<QObject> weak = object.toWeakRef();
    QVariant variant = QVariant::fromValue<QWeakPointer<QObject> >(weak);
    object->setProperty(QPERSISTENCE_SHARED_POINTER_PROPERTY.toLatin1(),
                        variant);
}

void setLastError(const QpError &error)
{
    QpError::setLastError(error);
}

}

}
