#ifndef QPERSISTENCE_DEFAULTSTORAGE_H
#define QPERSISTENCE_DEFAULTSTORAGE_H

#include "storage.h"
#include "error.h"

namespace Qp {

void setDatabase(const QSqlDatabase &database);
QSqlDatabase database();
bool adjustDatabaseSchema();
bool createCleanSchema();
QpError lastError();
#ifndef QP_NO_LOCKS
void enableLocks();
void addAdditionalLockInformationField(const QString &field, QVariant::Type type = QVariant::UserType);
#endif
#ifndef QP_NO_TIMESTAMPS
QDateTime databaseTime();
#endif

bool beginTransaction();
CommitResult commitOrRollbackTransaction();

void setSqlDebugEnabled(bool enable);

template<class T> int primaryKey(QSharedPointer<T> object) { return Qp::Private::primaryKey(object.data()); }
template<class T, class... Superclasses> void registerClass() { return QpStorage::defaultStorage()->registerClass<T, Superclasses...>(); }
template<class T> QpDao<T> *dataAccessObject() { return QpStorage::defaultStorage()->dataAccessObject<T>(); }
template<class T> QSharedPointer<T> read(int id) { return QpStorage::defaultStorage()->read<T>(id); }
template<class T> QList<QSharedPointer<T> > readAll() { return QpStorage::defaultStorage()->readAll<T>(); }
template<class T> int count() { return QpStorage::defaultStorage()->count<T>(); }
template<class T> QSharedPointer<T> create() { return QpStorage::defaultStorage()->create<T>(); }
template<class T> UpdateResult update(QSharedPointer<T> object) { return QpStorage::defaultStorage()->update(object); }
template<class T> bool remove(QSharedPointer<T> object) { return QpStorage::defaultStorage()->remove(object); }
template<class T> bool markAsDeleted(QSharedPointer<T> object) { return QpStorage::defaultStorage()->markAsDeleted(object); }
template<class T> bool undelete(QSharedPointer<T> object) { return QpStorage::defaultStorage()->undelete(object); }
template<class T> bool isDeleted(QSharedPointer<T> object) { return QpStorage::defaultStorage()->isDeleted(object); }
template<class T> SynchronizeResult synchronize(QSharedPointer<T> object) { return QpStorage::defaultStorage()->synchronize(object); }
template<class T> bool incrementNumericColumn(QSharedPointer<T> object, const QString &fieldName) { return QpStorage::defaultStorage()->incrementNumericColumn(object, fieldName); }
#ifndef QP_NO_TIMESTAMPS
template<class T> QList<QSharedPointer<T>> createdSince(const QDateTime &time) { return QpStorage::defaultStorage()->createdSince<T>(time); }
template<class T> QList<QSharedPointer<T>> updatedSince(const QDateTime &time) { return QpStorage::defaultStorage()->updatedSince<T>(time); }
template<class T> QDateTime creationTimeInDatabase(QSharedPointer<T> object) { return QpStorage::defaultStorage()->creationTimeInDatabase(object); }
template<class T> QDateTime creationTimeInInObject(QSharedPointer<T> object) { return QpStorage::defaultStorage()->creationTimeInInObject(object); }
template<class T> QDateTime updateTimeInDatabase(QSharedPointer<T> object) { return QpStorage::defaultStorage()->updateTimeInDatabase(object); }
template<class T> QDateTime updateTimeInObject(QSharedPointer<T> object) { return QpStorage::defaultStorage()->updateTimeInObject(object); }
#endif
#ifndef QP_NO_LOCKS
template<class T> QpLock tryLock(QSharedPointer<T> object, QHash<QString,QVariant> additionalInformation = QHash<QString,QVariant>())
 { return QpStorage::defaultStorage()->tryLock(object, additionalInformation); }
template<class T> QpLock unlock(QSharedPointer<T> object) { return QpStorage::defaultStorage()->unlock(object); }
template<class T> QpLock isLocked(QSharedPointer<T> object) { return QpStorage::defaultStorage()->isLocked(object); }
#endif

}

#endif // QPERSISTENCE_DEFAULTSTORAGE_H
