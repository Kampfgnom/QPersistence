#ifndef QPERSISTENCE_DEFAULTSTORAGE_H
#define QPERSISTENCE_DEFAULTSTORAGE_H

#include "storage.h"
#include "error.h"

namespace Qp {

QpStorage *defaultStorage();
void setDatabase(const QSqlDatabase &database);
QSqlDatabase database();
bool adjustDatabaseSchema();
bool createCleanSchema();
QpError lastError();
void addErrorHandler(QpAbstractErrorHandler *handler);
void clearErrorHandlers();
#ifndef QP_NO_LOCKS
void enableLocks();
bool unlockAllLocks();
void addAdditionalLockInformationField(const QString &field, QVariant::Type type = QVariant::UserType);
#endif
#ifndef QP_NO_TIMESTAMPS
QDateTime databaseTime();
#endif

void setSqlDebugEnabled(bool enable);

template<class T> int primaryKey(QSharedPointer<T> object) {
    return Qp::Private::primaryKey(object.data());
}
template<class T, class ... Superclasses> void registerClass() {
    return Qp::defaultStorage()->registerClass<T, Superclasses ...>();
}
template<class T> QpDataAccessObject<T> *dataAccessObject() {
    return Qp::defaultStorage()->dataAccessObject<T>();
}
template<class T> QSharedPointer<T> read(int id) {
    return Qp::defaultStorage()->read<T>(id);
}
template<class T> QList<QSharedPointer<T> > readAll(const QpSqlCondition &condition = QpSqlCondition()) {
    return Qp::defaultStorage()->readAll<T>(condition);
}
template<class T> int count(const QpSqlCondition &condition = QpSqlCondition()) {
    return Qp::defaultStorage()->count<T>(condition);
}
template<class T> QSharedPointer<T> create() {
    return Qp::defaultStorage()->create<T>();
}
template<class T> UpdateResult update(QSharedPointer<T> object) {
    return Qp::defaultStorage()->update(object);
}
template<class T> bool remove(QSharedPointer<T> object) {
    return Qp::defaultStorage()->remove(object);
}
template<class T> bool markAsDeleted(QSharedPointer<T> object) {
    return Qp::defaultStorage()->markAsDeleted(object);
}
template<class T> bool undelete(QSharedPointer<T> object) {
    return Qp::defaultStorage()->undelete(object);
}
template<class T> bool isDeleted(QSharedPointer<T> object) {
    return Qp::defaultStorage()->isDeleted(object);
}
template<class T> SynchronizeResult synchronize(QSharedPointer<T> object, QpDataAccessObjectBase::SynchronizeMode mode = QpDataAccessObjectBase::NormalMode) {
    return Qp::defaultStorage()->synchronize(object, mode);
}
template<class T> bool incrementNumericColumn(QSharedPointer<T> object, const QString &fieldName) {
    return Qp::defaultStorage()->incrementNumericColumn(object, fieldName);
}
#ifndef QP_NO_TIMESTAMPS
template<class T> QList<QSharedPointer<T> > createdSince(const QDateTime &time) {
    return Qp::defaultStorage()->createdSince<T>(time);
}
template<class T> QList<QSharedPointer<T> > updatedSince(const QDateTime &time) {
    return Qp::defaultStorage()->updatedSince<T>(time);
}
template<class T> QDateTime creationTime(QSharedPointer<T> object) {
    return Qp::defaultStorage()->creationTime(object);
}
template<class T> QDateTime creationTimeInDatabase(QSharedPointer<T> object) {
    return Qp::defaultStorage()->creationTimeInDatabase(object);
}
template<class T> QDateTime creationTimeInInObject(QSharedPointer<T> object) {
    return Qp::defaultStorage()->creationTimeInInObject(object);
}
template<class T> QDateTime updateTimeInDatabase(QSharedPointer<T> object) {
    return Qp::defaultStorage()->updateTimeInDatabase(object);
}
template<class T> QDateTime updateTimeInObject(QSharedPointer<T> object) {
    return Qp::defaultStorage()->updateTimeInObject(object);
}
#endif
#ifndef QP_NO_LOCKS
template<class T> bool isLocked(QSharedPointer<T> object, QpStorage::IsLockedOption option = QpStorage::LockStateFromLastSync)
{
    return Qp::defaultStorage()->isLocked(object, option);
}
template<class T> QpLock tryLock(QSharedPointer<T> object, QHash<QString,QVariant> additionalInformation = QHash<QString,QVariant>())
{
    return Qp::defaultStorage()->tryLock(object, additionalInformation);
}
template<class T> QpLock unlock(QSharedPointer<T> object) {
    return Qp::defaultStorage()->unlock(object);
}
template<class T> QpLock lockStatus(QSharedPointer<T> object) {
    return Qp::defaultStorage()->lockStatus(object);
}
#endif

}

#endif // QPERSISTENCE_DEFAULTSTORAGE_H
