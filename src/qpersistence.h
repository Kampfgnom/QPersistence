#ifndef QPERSISTENCE_H
#define QPERSISTENCE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "private.h"

class QpDataAccessObjectBase;
class QpLock;
class QpMetaObject;

namespace Qp {

enum SynchronizeResult : short {
    Error,
    Unchanged,
    Updated,
    LastSyncNewEnough,
    Removed,
    Deleted,
    RebaseConflict
};

enum UpdateResult : short {
    UpdateSuccess,
    UpdateConflict,
    UpdateError
};


template<class K, class V> void registerMappableTypes();
template<class T> void registerSetType();
template<class T> QSharedPointer<T> sharedFrom(const QObject *object);
template<typename T> QList<T> reversed( const QList<T> & in );
template<class Source, class Target>
QList<Target> castList(const QList<Source>& list);
template<class T, class O>
QList<QSharedPointer<T> > castList(const QList<QSharedPointer<O> >& list);
template<class T> QVariant variant(QSharedPointer<T> object);
inline int variantUserType(const QMetaObject &metaObject)
{
    return Qp::Private::variantCast(QSharedPointer<QObject>(), metaObject.className()).userType();
}
template<class T> int variantUserType(QSharedPointer<T>);
template<class T>
typename std::enable_if<std::is_base_of<QObject, T>::value, int>::type variantUserType();
template<class T>
typename std::enable_if<std::is_enum<T>::value, int>::type variantUserType();


/*******************************************************
 * Implementation
 */
template<class T>
QSharedPointer<T> sharedFrom(const T *object)
{
    return qSharedPointerCast<T>(Qp::Private::sharedFrom(object));
}

template <typename T>
QList<T> reversed( const QList<T> & in ) {
    QList<T> result;
    result.reserve( in.size() );
    std::reverse_copy( in.begin(), in.end(), std::back_inserter( result ) );
    return result;
}

template<class K, class V>
void registerMappableTypes()
{
    qRegisterMetaType<K>();
    qRegisterMetaType<V>();
    qRegisterMetaType<QMap<K,V> >();

    // Create converter
    Private::registerConverter<QMap<K,V> >(new Private::MapConverter<K,V>());

    if (!Private::canConvertFromSqlStoredVariant<K>())
        Private::registerConverter<K>(new Private::SqlConverter<K>());
    if (!Private::canConvertFromSqlStoredVariant<V>())
        Private::registerConverter<V>(new Private::SqlConverter<V>());
}

template<class T>
void registerSetType()
{
    qRegisterMetaType<T>();
    qRegisterMetaType<QSet<T> >();

    // Create converter
    Private::registerConverter<QSet<T> >(new Private::SetConverter<T>());

    if (!Private::canConvertFromSqlStoredVariant<T>())
        Private::registerConverter<T>(new Private::SqlConverter<T>());
}

template<class Target, class Source>
QList<Target> castList(const QList<Source>& list)
{
    QList<Target> result;
    foreach (Source s, list) result.append(static_cast<Target>(s));
    return result;
}

template<class Target, class Source>
QList<QSharedPointer<Target> > castList(const QList<QSharedPointer<Source> >& list)
{
    QList<QSharedPointer<Target> > result;
    result.reserve(list.size());
    foreach (QSharedPointer<Source> s, list) result.append(qSharedPointerCast<Target>(s));
    return result;
}
template<class T> QVariant variant(QSharedPointer<T> object)
{
    return QVariant::fromValue<QSharedPointer<T> >(object);
}

template<class T> int variantUserType(QSharedPointer<T>)
{
    return QVariant::fromValue<QSharedPointer<T> >(QSharedPointer<T>()).userType();
}

int variantUserType(const QMetaObject &metaObject);

template<class T>
typename std::enable_if<std::is_base_of<QObject, T>::value, int>::type variantUserType()
{
    return QVariant::fromValue<QSharedPointer<T> >(QSharedPointer<T>()).userType();
}

template<class T>
typename std::enable_if<std::is_enum<T>::value, int>::type variantUserType()
{
    return QVariant::fromValue<T>(static_cast<T>(0)).userType();
}

} // namespace Qp

#define QPERSISTENCE_PROPERTYMETADATA "QPERSISTENCE_PROPERTYMETADATA"
#define QPERSISTENCE_SQLFILTER "QPERSISTENCE_SQLFILTER"
#define QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION "reverserelation"


#endif // QPERSISTENCE_H
