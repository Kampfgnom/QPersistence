#include "qpersistence.h"

#include "metaobject.h"

#include <QDebug>

namespace QPersistence {

namespace Private {
QHash<QString, QPersistenceMetaObject> metaObjects;
QHash<QString, QHash<QString, QPersistenceAbstractDataAccessObject *> > daoPerConnectionAndMetaObject;
QHash<int, ConverterBase *> convertersByUserType;
QHash<QString, ConverterBase *> convertersByClassName;
}

QPersistenceMetaObject metaObject(const QString &className)
{
    return Private::metaObjects.value(className);
}

QList<QPersistenceMetaObject> metaObjects()
{
    return Private::metaObjects.values();
}

QList<QPersistenceAbstractDataAccessObject *> dataAccessObjects(const QString connectionName)
{
    return Private::daoPerConnectionAndMetaObject[connectionName].values();
}

QPersistenceAbstractDataAccessObject *dataAccessObject(const QMetaObject &metaObject, const QString &connectionName)
{

    QPersistenceAbstractDataAccessObject *dao = Private::daoPerConnectionAndMetaObject[connectionName].value(metaObject.className());
    if(dao)
        return dao;

    return Private::daoPerConnectionAndMetaObject[QString()].value(metaObject.className());
}

namespace Private {
void registerDataAccessObject(QPersistenceAbstractDataAccessObject *dataAccessObject,
                              const QMetaObject &metaObject,
                              const QString &connectionName)
{
    daoPerConnectionAndMetaObject[connectionName].insert(metaObject.className(), dataAccessObject);
    metaObjects.insert(metaObject.className(), QPersistenceMetaObject(metaObject));
}

void registerConverter(int variantType, ConverterBase *converter)
{
    convertersByUserType.insert(variantType, converter);
    convertersByClassName.insert(converter->className(), converter);
}

QObject *objectCast(const QVariant &variant)
{
    Q_ASSERT(convertersByUserType.contains(variant.userType()));

    return convertersByUserType.value(variant.userType())->convertObject(variant);
}

QList<QObject *> objectListCast(const QVariant &variant)
{
    Q_ASSERT(convertersByUserType.contains(variant.userType()));

    return convertersByUserType.value(variant.userType())->convertList(variant);
}

QVariant variantCast(QObject *object, const QString &classN)
{
    QString className = classN;
    if(className.isEmpty()) {
        Q_ASSERT(object);
        className = QLatin1String(object->metaObject()->className());
    }
    Q_ASSERT(convertersByClassName.contains(className));
    return convertersByClassName.value(className)->convertVariant(object);
}

QVariant variantListCast(QList<QObject *> objects, const QString &className)
{
    Q_ASSERT(convertersByClassName.contains(className));
    return convertersByClassName.value(className)->convertVariantList(objects);
}

}

}
