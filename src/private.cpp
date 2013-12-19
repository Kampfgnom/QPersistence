#include "private.h"

#include "metaproperty.h"
#include "databaseschema.h"

#include <QSharedPointer>
#include <QDebug>

namespace Qp {

namespace Private {

QHash<QString, QpDaoBase *> daoPerMetaObjectName;

QpDaoBase *dataAccessObject(const QMetaObject &metaObject)
{
    QpDaoBase *dao = Private::daoPerMetaObjectName.value(metaObject.className());
    if(dao)
        return dao;

    return Private::daoPerMetaObjectName.value(metaObject.className());
}

void registerDataAccessObject(QpDaoBase *dataAccessObject)
{
    daoPerMetaObjectName.insert(dataAccessObject->qpMetaObject().className(), dataAccessObject);
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
