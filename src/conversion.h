#ifndef QPERSISTENCE_CONVERSION_H
#define QPERSISTENCE_CONVERSION_H

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QRegularExpressionMatch>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include "dataaccessobject.h"

namespace Qp {

namespace Private {

class ConverterBase;

QSharedPointer<QObject> objectCast(const QVariant &variant);
QList<QSharedPointer<QObject> > objectListCast(const QVariant &variant);
QVariant variantCast(QSharedPointer<QObject> object, const QString &className);
QVariant variantListCast(QList<QSharedPointer<QObject> > objects, const QString &className);

QString convertToSqlStorableVariant(const QVariant &variant);
bool canConvertToSqlStorableVariant(const QVariant &variant);
template<class T>
bool canConvertToSqlStorableVariant()
{
    QVariant v = QVariant::fromValue<T>(T());
    if (static_cast<QMetaType::Type>(v.type()) != QMetaType::User)
        return true;

    return canConvertToSqlStorableVariant(v);
}

QVariant convertFromSqlStoredVariant(const QString &variant, QMetaType::Type type);
bool canConvertFromSqlStoredVariant(QMetaType::Type type);
template<class T>
bool canConvertFromSqlStoredVariant()
{
    QVariant v = QVariant::fromValue<T>(T());
    if (static_cast<QMetaType::Type>(v.type()) != QMetaType::User)
        return true;

    return canConvertFromSqlStoredVariant(static_cast<QMetaType::Type>(v.userType()));
}


class ConverterBase : public QObject
{
public:
    ConverterBase(QObject *parent) : QObject(parent) {}
    virtual ~ConverterBase() {}
    virtual QList<QSharedPointer<QObject> > convertList(const QVariant &variant) const { Q_UNUSED(variant) return QList<QSharedPointer<QObject> >(); }
    virtual QSharedPointer<QObject> convertObject(const QVariant &variant) const { Q_UNUSED(variant) return QSharedPointer<QObject>(); }
    virtual QVariant convertVariant(QSharedPointer<QObject> object) const { Q_UNUSED(object) return QVariant(); }
    virtual QVariant convertVariantList(QList<QSharedPointer<QObject> > objects) const { Q_UNUSED(objects) return QVariant(); }
    virtual QString className() const { return QString(); }
    virtual QString convertToSqlStorableValue(const QVariant &variant) const { Q_UNUSED(variant) return QString(); }
    virtual QVariant convertFromSqlStorableValue(const QString &value) const { Q_UNUSED(value) return QVariant(); }
};

template<class O>
class ObjectConverter : public ConverterBase
{
public:
    ObjectConverter(QObject *parent = 0) : ConverterBase(parent) {}
    QList<QSharedPointer<QObject> > convertList(const QVariant &variant) const
    {
        QList<QSharedPointer<O> > list = variant.value<QList<QSharedPointer<O> > >();
        QList<QSharedPointer<QObject> > result;
        Q_FOREACH(QSharedPointer<O> object, list) result.append(object);
        return result;
    }

    QSharedPointer<QObject> convertObject(const QVariant &variant) const
    {
        return variant.value<QSharedPointer<O>  >();
    }

    QVariant convertVariant(QSharedPointer<QObject> object) const
    {
        return QVariant::fromValue<QSharedPointer<O>  >(qSharedPointerCast<O>(object));
    }

    QVariant convertVariantList(QList<QSharedPointer<QObject> > objects) const
    {
        QList<QSharedPointer<O>  > result;
        Q_FOREACH(QSharedPointer<QObject> object, objects) result.append(qSharedPointerCast<O>(object));
        return QVariant::fromValue<QList<QSharedPointer<O>  > >(result);
    }

    QString className() const
    {
        return QLatin1String(O::staticMetaObject.className());
    }
};

template<typename T>
class SqlConverter : public Qp::Private::ConverterBase
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

public:
    MapConverter(QObject *parent = 0) : ConverterBase(parent) {}
    QString convertToSqlStorableValue(const QVariant &variant) const
    {
        Q_ASSERT(canConvertToSqlStorableVariant<K>());
        Q_ASSERT(canConvertToSqlStorableVariant<V>());

        QMap<K, V> map = variant.value<QMap<K,V> >();
        QMapIterator<K,V> it(map);
        QStringList result;
        while (it.hasNext()) {
            it.next();
            QVariant keyVariant = QVariant::fromValue<K>(it.key());
            QVariant valueVariant = QVariant::fromValue<V>(it.value());
            QString key = Private::convertToSqlStorableVariant(keyVariant);
            QString value = Private::convertToSqlStorableVariant(valueVariant);

            result.append(QString("%1=%2")
                          .arg(key)
                          .arg(value));
        }
        return result.join(';');
    }

    QVariant convertFromSqlStorableValue(const QString &value) const
    {
        QMap<K, V> result;

        if (value.isEmpty())
            return QVariant::fromValue<QMap<K,V> >(result);

        Q_ASSERT(canConvertFromSqlStoredVariant<K>());
        Q_ASSERT(canConvertFromSqlStoredVariant<V>());

        QStringList list = value.split(';');
        QMetaType::Type keyType = static_cast<QMetaType::Type>(QVariant::fromValue<K>(K()).userType());
        QMetaType::Type valueType = static_cast<QMetaType::Type>(QVariant::fromValue<V>(V()).userType());

        foreach (QString string, list) {
            QStringList list2 = string.split('=');
            Q_ASSERT(list2.size() == 2);
            QVariant keyVariant = Private::convertFromSqlStoredVariant(list2.at(0), keyType);
            QVariant valueVariant = Private::convertFromSqlStoredVariant(list2.at(1), valueType);

            K key = keyVariant.value<K>();
            V v = valueVariant.value<V>();
            result.insert(key, v);
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
        if (static_cast<QMetaType::Type>(v.type()) == QMetaType::User) {
            Q_ASSERT(canConvertToSqlStorableVariant<T>());
        }

        QSet<T> set = variant.value<QSet<T> >();
        QSetIterator<T> it(set);
        QStringList result;
        while (it.hasNext()) {
            result.append(Private::convertToSqlStorableVariant(QVariant::fromValue<T>(it.next())));
        }
        return result.join(';');
    }

    QVariant convertFromSqlStorableValue(const QString &value) const
    {
        QVariant v = QVariant::fromValue<T>(T());
        if (static_cast<QMetaType::Type>(v.type()) == QMetaType::User) {
            Q_ASSERT(canConvertToSqlStorableVariant<T>());
        }

        QStringList list = value.split(';');
        QSet<T> result;
        QMetaType::Type type = static_cast<QMetaType::Type>(QVariant::fromValue<T>(T()).userType());

        foreach (QString string, list) {
            QVariant variant = Private::convertFromSqlStoredVariant(string, type);
            result.insert(variant.value<T>());
        }
        return QVariant::fromValue<QSet<T> >(result);
    }
};


void registerConverter(int variantType, ConverterBase *converter);

template<typename T>
void registerConverter(Private::ConverterBase *converter)
{
    QVariant v = QVariant::fromValue<T>(T());
    if (static_cast<QMetaType::Type>(v.type()) == QMetaType::User) {
        Private::registerConverter(v.userType(), converter);
    }
}

} // namespace Private

} // namespace Qp

#endif // QPERSISTENCE_CONVERSION_H
