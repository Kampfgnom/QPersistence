#ifndef QPERSISTENCE_H
#define QPERSISTENCE_H

#include <QtCore/QHash>
#include <QtCore/QVariant>

#define QPERSISTENCE_PRIMARYKEY "QPERSISTENCE_PRIMARYKEY"
#define QPERSISTENCE_SQL_TABLENAME "QPERSISTENCE_SQL_TABLENAME"
#define QPERSISTENCE_REST_COLLECTIONNAME "QPERSISTENCE_REST_COLLECTIONNAME"

#define QPERSISTENCE_PROPERTYMETADATA "QPERSISTENCE_PROPERTYMETADATA"
#define QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION "reverserelation"
#define QPERSISTENCE_PROPERTYMETADATA_AUTOINCREMENTED "autoincremented"
#define QPERSISTENCE_PROPERTYMETADATA_READONLY "readonly"
#define QPERSISTENCE_PROPERTYMETADATA_SQL_COLUMNNAME "columnname"

#define QPERSISTENCE_TRUE "true"
#define QPERSISTENCE_FALSE "false"

class QPersistenceMetaObject;
class QPersistenceAbstractDataAccessObject;

namespace QPersistence {

template<class T>
void registerDataAccessObject(QPersistenceAbstractDataAccessObject *dataAccessObject, const QString &connectionName = QString());

QPersistenceMetaObject metaObject(const QString &className);
QList<QPersistenceMetaObject> metaObjects();

template<class T>
QPersistenceAbstractDataAccessObject *dataAccessObject(const QString &connectionName = QString());
QPersistenceAbstractDataAccessObject *dataAccessObject(const QMetaObject &metaObject, const QString &connectionName = QString());
QList<QPersistenceAbstractDataAccessObject *> dataAccessObjects(const QString connectionName = QString());



// Private
namespace Private {

class ConverterBase;

QObject *objectCast(const QVariant &variant);
QList<QObject *> objectListCast(const QVariant &variant);
QVariant variantCast(QObject *object, const QString &className = QString());
QVariant variantListCast(QList<QObject *> objects, const QString &className);


class ConverterBase : public QObject
{
public:
    ConverterBase(QObject *parent = 0) : QObject(parent) {}
    virtual ~ConverterBase() {}
    virtual QList<QObject *> convertList(const QVariant &variant) const = 0;
    virtual QObject *convertObject(const QVariant &variant) const = 0;
    virtual QVariant convertVariant(QObject *object) const = 0;
    virtual QVariant convertVariantList(QList<QObject *> objects) const = 0;
    virtual QString className() const = 0;
};

template<class O>
class Converter : public ConverterBase
{
public:
    Converter(QObject *parent = 0) : ConverterBase(parent) {}
    QList<QObject *> convertList(const QVariant &variant) const
    {
        QList<O *> list = variant.value<QList<O *> >();
        QList<QObject *> result;
        Q_FOREACH(O *object, list) result.append(object);
        return result;
    }

    QObject *convertObject(const QVariant &variant) const
    {
        return variant.value<O *>();
    }

    QVariant convertVariant(QObject *object) const
    {
        return QVariant::fromValue<O *>(static_cast<O *>(object));
    }

    QVariant convertVariantList(QList<QObject *> objects) const
    {
        QList<O *> result;
        Q_FOREACH(QObject *object, objects) result.append(static_cast<O *>(object));
        return QVariant::fromValue<QList<O *> >(result);
    }

    QString className() const
    {
        return QLatin1String(O::staticMetaObject.className());
    }
};

void registerConverter(int variantType, ConverterBase *converter);
void registerDataAccessObject(QPersistenceAbstractDataAccessObject *dataAccessObject,
                              const QMetaObject &metaObject,
                              const QString &connectionName);


} // namespace Private

template<class T>
void registerDataAccessObject(QPersistenceAbstractDataAccessObject *dataAccessObject, const QString &connectionName)
{
    qRegisterMetaType<T*>();
    qRegisterMetaType<QList<T*> >();

    // Register meta object
    Private::registerDataAccessObject(dataAccessObject,
                                      T::staticMetaObject,
                                      connectionName);

    // Create converter
    static QObject guard;
    Private::Converter<T> *converter = new Private::Converter<T>(&guard);

    // Register converter for type
    QVariant v = QVariant::fromValue<T *>(nullptr);
    Private::registerConverter(v.userType(), converter);

    // Register converter for list type
    v = QVariant::fromValue<QList<T *> >(QList<T *>());
    Private::registerConverter(v.userType(), converter);
}

template<class T>
QPersistenceAbstractDataAccessObject *dataAccessObject(const QString &connectionName)
{
    return dataAccessObject(T::staticMetaObject.className(), connectionName);
}

} // namespace QPersistence

#endif // QPERSISTENCE_H
