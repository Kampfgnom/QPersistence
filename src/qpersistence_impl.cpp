#include "qpersistence.h"

#include "conversion.h"
#include "lock.h"
#include "metaproperty.h"
#include "private.h"
#include "relationresolver.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>
#include <QtCore/QDebug>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

namespace Qp {

template<class T>
void registerClass()
{
    qRegisterMetaType<QSharedPointer<T> >();
    qRegisterMetaType<QList<QSharedPointer<T> > >();

    new QpDao<T>(Private::GlobalGuard());

    // Create converter
    Private::ObjectConverter<T> *converter = new Private::ObjectConverter<T>(Private::GlobalGuard());

    // Register converter for type
    Private::registerConverter<QList<QSharedPointer<T> > >(converter);

    // Register converter for list type
    Private::registerConverter<QSharedPointer<T> >(converter);
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

template<class T> QSharedPointer<T> read(int id)
{
    return Qp::dataAccessObject<T>()->read(id);
}

template<class T>
QList<QSharedPointer<T> > readAll()
{
    return dataAccessObject<T>()->readAllObjects();
}

template<class T>
int count()
{
    return dataAccessObject<T>()->count();
}

template<class T>
QSharedPointer<T> create()
{
    return qSharedPointerCast<T>(dataAccessObject<T>()->createObject());
}

template<class T>
QpDao<T> *dataAccessObject()
{
    return static_cast<QpDao<T> *>(QpDaoBase::forClass(T::staticMetaObject));
}

template<class T>
UpdateResult update(QSharedPointer<T> object)
{
    beginTransaction();
    Qp::UpdateResult result = QpDaoBase::forClass(*object->metaObject())->updateObject(object);
    if(result == Qp::UpdateConflict) {
        Qp::database().rollback();
        return Qp::UpdateConflict;
    }

    CommitResult commitResult = commitOrRollbackTransaction();
    if(commitResult == CommitSuccessful)
        return result;

    return Qp::UpdateError;
}

template<class T>
SynchronizeResult synchronize(QSharedPointer<T> object)
{
    return QpDaoBase::forClass(*object->metaObject())->synchronizeObject(object);
}

template<class T>
QList<QSharedPointer<T> > createdSince(const QDateTime &time)
{
    return castList<T>(QpDaoBase::forClass(T::staticMetaObject)->createdSince(time));
}

template<class T>
QList<QSharedPointer<T> > updatedSince(const QDateTime &time)
{
    return castList<T>(QpDaoBase::forClass(T::staticMetaObject)->updatedSince(time));
}

template<class T>
bool remove(QSharedPointer<T> object)
{
    beginTransaction();
    QpDaoBase::forClass(*object->metaObject())->removeObject(object);
    return commitOrRollbackTransaction() == CommitSuccessful;
}

template<class T>
QSharedPointer<T> sharedFrom(const T *object)
{
    return qSharedPointerCast<T>(Qp::Private::sharedFrom(object));
}

template<class T>
int primaryKey(QSharedPointer<T> object)
{
    return Qp::Private::primaryKey(object.data());
}

template<class T>
bool isDeleted(QSharedPointer<T> object)
{
    return Qp::Private::isDeleted(object.data());
}

template<class T>
bool markAsDeleted(QSharedPointer<T> object)
{
    return QpDaoBase::forClass(*object->metaObject())->markAsDeleted(object);
}

#ifndef QP_NO_TIMESTAMPS
QDateTime dateFromDouble(double value);

template<class T> QDateTime creationTimeInDatabase(QSharedPointer<T> object)
{
    return dateFromDouble(Qp::Private::creationTimeInDatabase(object.data()));
}

template<class T> QDateTime updateTimeInDatabase(QSharedPointer<T> object)
{
    return dateFromDouble(Qp::Private::updateTimeInDatabase(object.data()));
}

template<class T> QDateTime updateTimeInObject(QSharedPointer<T> object)
{
    return dateFromDouble(Qp::Private::updateTimeInObject(object.data()));
}      
#endif

#ifndef QP_NO_LOCKS
template<class T> QpLock tryLock(QSharedPointer<T> object, QHash<QString,QVariant> additionalInformation)
{
    return QpLock::tryLock(qSharedPointerCast<QObject>(object), additionalInformation);
}

template<class T> QpLock unlock(QSharedPointer<T> object)
{
    return QpLock::unlock(qSharedPointerCast<QObject>(object));
}

template<class T> QpLock isLocked(QSharedPointer<T> object)
{
    return QpLock::isLocked(qSharedPointerCast<QObject>(object));
}
#endif

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
