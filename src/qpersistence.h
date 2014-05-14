#ifndef QPERSISTENCE_H
#define QPERSISTENCE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "private.h"

template<class T>
class QpDao;
class QpDaoBase;
class QpError;
class QpLock;
class QpMetaObject;

namespace Qp {

enum CommitResult {
    RollbackSuccessful,
    RollbackFailed,
    CommitSuccessful,
    CommitFailed
};

enum SynchronizeResult : short {
    Error,
    Unchanged,
    Updated,
    LastSyncNewEnough,
    Removed,
    Deleted
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
    Private::registerConverter<QMap<K,V> >(new Private::MapConverter<K,V>(Private::GlobalGuard()));

    if (!Private::canConvertFromSqlStoredVariant<K>())
        Private::registerConverter<K>(new Private::SqlConverter<K>(Private::GlobalGuard()));
    if (!Private::canConvertFromSqlStoredVariant<V>())
        Private::registerConverter<V>(new Private::SqlConverter<V>(Private::GlobalGuard()));
}

template<class T>
void registerSetType()
{
    qRegisterMetaType<T>();
    qRegisterMetaType<QSet<T> >();

    // Create converter
    Private::registerConverter<QSet<T> >(new Private::SetConverter<T>(Private::GlobalGuard()));

    if (!Private::canConvertFromSqlStoredVariant<T>())
        Private::registerConverter<T>(new Private::SqlConverter<T>(Private::GlobalGuard()));
}

template<class Target, class Source>
QList<Target> castList(const QList<Source>& list)
{
    QList<Target> result;
    foreach(Source s, list) result.append(static_cast<Target>(s));
    return result;
}

template<class Target, class Source>
QList<QSharedPointer<Target> > castList(const QList<QSharedPointer<Source> >& list)
{
    QList<QSharedPointer<Target> > result;
    result.reserve(list.size());
    foreach(QSharedPointer<Source> s, list) result.append(qSharedPointerCast<Target>(s));
    return result;
}

} // namespace Qp

#define QPERSISTENCE_PROPERTYMETADATA "QPERSISTENCE_PROPERTYMETADATA"
#define QPERSISTENCE_SQLFILTER "QPERSISTENCE_SQLFILTER"
#define QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION "reverserelation"


#endif // QPERSISTENCE_H
