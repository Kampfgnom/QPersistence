#include "private.h"

#include "metaproperty.h"
#include "databaseschema.h"

#include <QSharedPointer>
#include <QDebug>

namespace Qp {

namespace Private {

QHash<QString, QpMetaObject> _metaObjects;
QHash<QString, QpDaoBase *> daoPerMetaObjectName;

QpMetaObject metaObject(const QString &className)
{
    Q_ASSERT_X(Private::_metaObjects.contains(className),
               Q_FUNC_INFO,
               QString("The '%1' class has not been registered.")
               .arg(className)
               .toLatin1());

    return Private::_metaObjects.value(className);
}

QList<QpMetaObject> metaObjects()
{
    return Private::_metaObjects.values();
}

QpDaoBase *dataAccessObject(const QMetaObject &metaObject)
{
    QpDaoBase *dao = Private::daoPerMetaObjectName.value(metaObject.className());
    if(dao)
        return dao;

    return Private::daoPerMetaObjectName.value(metaObject.className());
}

void registerDataAccessObject(QpDaoBase *dataAccessObject,
                              const QMetaObject &metaObject)
{
    daoPerMetaObjectName.insert(metaObject.className(), dataAccessObject);
    _metaObjects.insert(metaObject.className(), dataAccessObject->qpMetaObject());
}

QList<QpDaoBase *> dataAccessObjects()
{
    return Private::daoPerMetaObjectName.values();
}

int primaryKey(QObject *object)
{
    return object->property("_Qp_ID").toInt();
}

void setPrimaryKey(QObject *object, int key)
{
    object->setProperty("_Qp_ID",key);
}

void enableSharedFromThis(QSharedPointer<QObject> object)
{
    QWeakPointer<QObject> weak = object.toWeakRef();
    QVariant variant = QVariant::fromValue<QWeakPointer<QObject> >(weak);
    object->setProperty(QPERSISTENCE_SHARED_POINTER_PROPERTY.toLatin1(),
                        variant);
}

}

}
