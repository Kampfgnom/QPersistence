#ifndef QPERSISTENCE_H
#define QPERSISTENCE_H

#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>
#include <QtCore/QDebug>

#include <QPersistenceAbstractDataAccessObject.h>

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

template<class K, class V>
void registerMappableTypes();

QPersistenceMetaObject metaObject(const QString &className);
QList<QPersistenceMetaObject> metaObjects();

template<class T>
QPersistenceAbstractDataAccessObject *dataAccessObject(const QString &connectionName = QString());
QPersistenceAbstractDataAccessObject *dataAccessObject(const QMetaObject &metaObject, const QString &connectionName = QString());
QList<QPersistenceAbstractDataAccessObject *> dataAccessObjects(const QString connectionName = QString());

template<class T>
QList<T *> readAll(const QString &connectionName = QString());
template<class T>
int count(const QString &connectionName = QString());
template<class T>
T *create(const QString &connectionName = QString());
bool insert(QObject *object, const QString &connectionName = QString());
bool update(QObject *object, const QString &connectionName = QString());
bool remove(QObject *object, const QString &connectionName = QString());

template<class Source, class Target>
QList<Target> castList(const QList<Source>& list)
{
    QList<Target> result;
    Q_FOREACH(Source s, list) result.append(static_cast<Target>(s));
    return result;
}


template<class T>
QList<T *> castList(const QList<QObject *>& list)
{
    return castList<QObject *, T *>(list);
}

// Private
namespace Private {

class ConverterBase;

QObject *objectCast(const QVariant &variant);
QList<QObject *> objectListCast(const QVariant &variant);
QVariant variantCast(QObject *object, const QString &className = QString());
QVariant variantListCast(QList<QObject *> objects, const QString &className);

QString convertToSqlStorableVariant(const QVariant &variant);
bool canConvertToSqlStorableVariant(const QVariant &variant);
template<class T>
bool canConvertToSqlStorableVariant()
{
    QVariant v = QVariant::fromValue<T>(T());
    return canConvertToSqlStorableVariant(v);
}

QVariant convertFromSqlStoredVariant(const QString &variant, QMetaType::Type type);
bool canConvertFromSqlStoredVariant(QMetaType::Type type);
template<class T>
bool canConvertFromSqlStoredVariant()
{
    QVariant v = QVariant::fromValue<T>(T());
    return canConvertFromSqlStoredVariant(static_cast<QMetaType::Type>(v.userType()));
}


class ConverterBase : public QObject
{
public:
    ConverterBase(QObject *parent = 0) : QObject(parent) {}
    virtual ~ConverterBase() {}
    virtual QList<QObject *> convertList(const QVariant &variant) const { Q_UNUSED(variant) return QList<QObject *>(); }
    virtual QObject *convertObject(const QVariant &variant) const { Q_UNUSED(variant) return nullptr; }
    virtual QVariant convertVariant(QObject *object) const { Q_UNUSED(object) return QVariant(); }
    virtual QVariant convertVariantList(QList<QObject *> objects) const { Q_UNUSED(objects) return QVariant(); }
    virtual QString className() const { return QString(); }
    virtual QString convertToSqlStorableValue(const QVariant &variant) const { Q_UNUSED(variant) return QString(); }
    virtual QVariant convertFromSqlStorableValue(const QString &value) const { Q_UNUSED(value) return QVariant(); }
};

template<class O>
class ObjectConverter : public ConverterBase
{
public:
    ObjectConverter(QObject *parent = 0) : ConverterBase(parent) {}
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

template<typename T>
class SqlConverter : public QPersistence::Private::ConverterBase
{
public:
    SqlConverter(QObject *parent = 0) : ConverterBase(parent) {}
    QString convertToSqlStorableValue(const QVariant &variant) const
    {
        return variant.value<QString>();
    }

    QVariant convertFromSqlStorableValue(const QString &string) const
    {
        return QVariant(string).value<T>();
    }
};

template<typename K, typename V>
class MapConverter : public ConverterBase
{
    static const QRegularExpression KEYVALUEREGEXP;

public:
    MapConverter(QObject *parent = 0) : ConverterBase(parent) {}
    QString convertToSqlStorableValue(const QVariant &variant) const
    {
        Q_ASSERT(canConvertToSqlStorableVariant<K>());
        Q_ASSERT(canConvertToSqlStorableVariant<V>());

        QMap<K, V> map = variant.value<QMap<K,V> >();
        QMapIterator<K,V> it(map);
        QStringList result;
        while(it.hasNext()) {
            it.next();
            QVariant keyVariant = QVariant::fromValue<K>(it.key());
            QVariant valueVariant = QVariant::fromValue<V>(it.value());
            QString key = Private::convertToSqlStorableVariant(keyVariant);
            QString value = Private::convertToSqlStorableVariant(valueVariant);
            qDebug() << key;
            qDebug() << value;

            result.append(QString("%1=%2")
                          .arg(key)
                          .arg(value));
        }
        return result.join(';');
    }

    QVariant convertFromSqlStorableValue(const QString &value) const
    {
        Q_ASSERT(canConvertFromSqlStoredVariant<K>());
        Q_ASSERT(canConvertFromSqlStoredVariant<V>());

        QStringList list = value.split(';');
        QMap<K, V> result;
        QMetaType::Type keyType = static_cast<QMetaType::Type>(QVariant::fromValue<K>(K()).userType());
        QMetaType::Type valueType = static_cast<QMetaType::Type>(QVariant::fromValue<V>(V()).userType());

        qDebug() << "value" << value;
        qDebug() << "list" << list;
        foreach(QString string, list) {
            QRegularExpressionMatch match = KEYVALUEREGEXP.match(string);
            if(match.hasMatch()) {
                QString keyString = match.captured(1);
                QString valueString = match.captured(2);
                qDebug() << "keyString" << keyString;
                qDebug() << "valueString" << valueString;
                QVariant keyVariant = Private::convertFromSqlStoredVariant(keyString, keyType);
                QVariant valueVariant = Private::convertFromSqlStoredVariant(valueString, valueType);
                qDebug() << "keyVariant" << keyVariant;
                qDebug() << "valueVariant" << valueVariant;

                K key = keyVariant.value<K>();
                V v = valueVariant.value<V>();
                qDebug() << "key" << key;
                qDebug() << "v" << v;
                result.insert(key, v);
            }
        }
        return QVariant::fromValue<QMap<K,V> >(result);
    }
};


template<typename T>
class SetConverter : public ConverterBase
{
public:
    SetConverter(QObject *parent = 0) : ConverterBase(parent) {}
    QString convertToSqlStorableValue(const QVariant &variant) const
    {
        QVariant v = QVariant::fromValue<T>(T());
        if(static_cast<QMetaType::Type>(v.type()) == QMetaType::User) {
            Q_ASSERT(canConvertToSqlStorableVariant<T>());
        }

        QSet<T> set = variant.value<QSet<T> >();
        QSetIterator<T> it(set);
        QStringList result;
        while(it.hasNext()) {
            result.append(Private::convertToSqlStorableVariant(QVariant::fromValue<T>(it.next())));
        }
        return result.join(';');
    }

    QVariant convertFromSqlStorableValue(const QString &value) const
    {
        QVariant v = QVariant::fromValue<T>(T());
        if(static_cast<QMetaType::Type>(v.type()) == QMetaType::User) {
            Q_ASSERT(canConvertToSqlStorableVariant<T>());
        }

        QStringList list = value.split(';');
        QSet<T> result;
        QMetaType::Type type = static_cast<QMetaType::Type>(QVariant::fromValue<T>(T()).userType());

        foreach(QString string, list) {
            QVariant variant = Private::convertFromSqlStoredVariant(string, type);
            result.insert(variant.value<T>());
        }
        return QVariant::fromValue<QSet<T> >(result);
    }
};

template<typename K, typename V>
const QRegularExpression MapConverter<K,V>::KEYVALUEREGEXP("(\\w+)=(\\w+)");


void registerConverter(int variantType, ConverterBase *converter);
void registerDataAccessObject(QPersistenceAbstractDataAccessObject *dataAccessObject,
                              const QMetaObject &metaObject,
                              const QString &connectionName);

} // namespace Private

template<typename T>
void registerConverter(Private::ConverterBase *converter)
{
    QVariant v = QVariant::fromValue<T>(T());
    if(static_cast<QMetaType::Type>(v.type()) == QMetaType::User) {
        Private::registerConverter(v.userType(), converter);
    }
}

template<class T>
QList<T *> readAll(const QString &connectionName)
{
    return castList<T>(dataAccessObject<T>(connectionName)->readAllObjects());
}

template<class T>
int count(const QString &connectionName)
{
    return dataAccessObject<T>(connectionName)->count();
}

template<class T>
T *create(const QString &connectionName)
{
    return static_cast<T *>(dataAccessObject<T>(connectionName)->createObject());
}

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
    Private::ObjectConverter<T> *converter = new Private::ObjectConverter<T>(&guard);

    // Register converter for type
    registerConverter<QList<T *> >(converter);

    // Register converter for list type
    registerConverter<T *>(converter);
}

template<class T>
QPersistenceAbstractDataAccessObject *dataAccessObject(const QString &connectionName)
{
    return dataAccessObject(T::staticMetaObject, connectionName);
}

template<class K, class V>
void registerMappableTypes()
{
    qRegisterMetaType<QMap<K,V> >();
    qRegisterMetaType<K>();
    qRegisterMetaType<V>();

    // Create converter
    static QObject guard;
    registerConverter<QMap<K,V> >(new Private::MapConverter<K,V>(&guard));

    if(!Private::canConvertFromSqlStoredVariant<K>())
        registerConverter<K>(new Private::SqlConverter<K>(&guard));
    if(!Private::canConvertFromSqlStoredVariant<V>())
        registerConverter<V>(new Private::SqlConverter<V>(&guard));
}

template<class T>
void registerSetType()
{
    qRegisterMetaType<T>();
    qRegisterMetaType<QSet<T> >();

    // Create converter
    static QObject guard;
    registerConverter<QSet<T> >(new Private::SetConverter<T>(&guard));

    if(!Private::canConvertFromSqlStoredVariant<T>())
        registerConverter<T>(new Private::SqlConverter<T>(&guard));
}

} // namespace QPersistence

#endif // QPERSISTENCE_H
