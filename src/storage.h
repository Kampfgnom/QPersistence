#ifndef QPERSISTENCE_STORAGE_H
#define QPERSISTENCE_STORAGE_H

#include "defines.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QDebug>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "qpersistence.h"
#include "private.h"
#include "sqldataaccessobjecthelper.h"
#include "databaseschema.h"
#include "lock.h"

class QSqlDatabase;
class QpError;
class QpAbstractErrorHandler;

class QpStorageData;
class QpStorage : public QObject
{
    Q_OBJECT
public:
    static QpStorage *defaultStorage();

    explicit QpStorage(QObject *parent = 0);
    ~QpStorage();

    QSqlDatabase database();
    void setDatabase(const QSqlDatabase &database);
    void setSqlDebugEnabled(bool enable);
    bool adjustDatabaseSchema();
    bool createCleanSchema();

    QpError lastError() const;
    void setLastError(QpError error);
    void addErrorHandler(QpAbstractErrorHandler *handler);
    void clearErrorHandlers();

    bool beginTransaction();
    Qp::CommitResult commitOrRollbackTransaction();

    void startBulkDatabaseQueries();
    void commitBulkDatabaseQueries();

    template<class T, class... Superclasses> void registerClass();

    QList<QpDaoBase *> dataAccessObjects();
    QpDaoBase *dataAccessObject(const QMetaObject metaObject) const;
    template<class T> QpDao<T> *dataAccessObject();
    template<class T> int primaryKey(QSharedPointer<T> object);
    template<class T> QSharedPointer<T> read(int id);
    template<class T> QList<QSharedPointer<T> > readAll();
    template<class T> int count();
    template<class T> QSharedPointer<T> create();
    template<class T> Qp::UpdateResult update(QSharedPointer<T> object);
    template<class T> bool remove(QSharedPointer<T> object);
    template<class T> bool markAsDeleted(QSharedPointer<T> object);
    template<class T> bool undelete(QSharedPointer<T> object);
    template<class T> bool isDeleted(QSharedPointer<T> object);
    template<class T> Qp::SynchronizeResult synchronize(QSharedPointer<T> object);
    template<class T> bool incrementNumericColumn(QSharedPointer<T> object, const QString &fieldName);
#ifndef QP_NO_TIMESTAMPS
    QDateTime databaseTime();
    double databaseTimeInternal();
    template<class T> QList<QSharedPointer<T>> createdSince(const QDateTime &time);
    template<class T> QList<QSharedPointer<T>> updatedSince(const QDateTime &time);
    template<class T> QDateTime creationTimeInDatabase(QSharedPointer<T> object);
    template<class T> QDateTime creationTimeInInObject(QSharedPointer<T> object);
    template<class T> QDateTime updateTimeInDatabase(QSharedPointer<T> object);
    template<class T> QDateTime updateTimeInObject(QSharedPointer<T> object);
    double creationTimeInDatabase(QObject *object);
    double creationTimeInInObject(QObject *object);
    double updateTimeInDatabase(QObject *object);
    double updateTimeInObject(QObject *object);
#endif
#ifndef QP_NO_LOCKS
    enum IsLockedOption { LockStateFromDatabase, LockStateFromLastSync };

    void enableLocks();
    bool isLocksEnabled();
    bool unlockAllLocks();
    void addAdditionalLockInformationField(const QString &name, QVariant::Type type = QVariant::UserType);
    QHash<QString, QVariant::Type> additionalLockInformationFields();
    template<class T> bool isLocked(QSharedPointer<T> object, IsLockedOption option = LockStateFromLastSync);
    template<class T> QpLock tryLock(QSharedPointer<T> object, QHash<QString,QVariant> additionalInformation = QHash<QString,QVariant>());
    template<class T> QpLock unlock(QSharedPointer<T> object);
    template<class T> QpLock lockStatus(QSharedPointer<T> object);
#endif

    QpSqlDataAccessObjectHelper *sqlDataAccessObjectHelper() const;
    void enableStorageFrom(QObject *object);
    static QpStorage *forObject(const QObject *object);

private:
    void registerDataAccessObject(QpDaoBase *dao, const QMetaObject *metaObject);
    QExplicitlySharedDataPointer<QpStorageData> data;
};
Q_DECLARE_METATYPE(QpStorage*)

template <class T>
void registerMetaType() {
    qRegisterMetaType<QSharedPointer<T> >();
    qRegisterMetaType<QList<QSharedPointer<T> > >();

    // Create converter
    Qp::Private::ObjectConverter<T> *converter = new Qp::Private::ObjectConverter<T>(Qp::Private::GlobalGuard());

    // Register converter for type
    Qp::Private::registerConverter<QList<QSharedPointer<T> > >(converter);

    // Register converter for list type
    Qp::Private::registerConverter<QSharedPointer<T> >(converter);
}

template<class T, class... Superclasses>
void QpStorage::registerClass()
{
    QpDao<T> *dao = new QpDao<T>(this);
    registerDataAccessObject(dao, &T::staticMetaObject);

    registerMetaType<T>();
    int _[] = {0, (registerMetaType<Superclasses>(), 0)...}; // I AM CRAAAAZY
    // http://stackoverflow.com/questions/12515616/expression-contains-unexpanded-parameter-packs/12515637#12515637
    Q_UNUSED(_)
}

template<class T> QSharedPointer<T> QpStorage::read(int id)
{
    return dataAccessObject<T>()->read(id);
}

template<class T>
QList<QSharedPointer<T> > QpStorage::readAll()
{
    return dataAccessObject<T>()->readAllObjects();
}

template<class T>
int QpStorage::count()
{
    return dataAccessObject<T>()->count();
}

template<class T>
QSharedPointer<T> QpStorage::create()
{
    return qSharedPointerCast<T>(dataAccessObject<T>()->createObject());
}

template<class T>
QpDao<T> *QpStorage::dataAccessObject()
{
    return static_cast<QpDao<T> *>(dataAccessObject(T::staticMetaObject));
}

template<class T>
bool QpStorage::incrementNumericColumn(QSharedPointer<T> object, const QString &fieldName)
{
    QpDao<T> *dao = dataAccessObject<T>();
    if(!dao->incrementNumericColumn(object, fieldName))
        return false;

    return dao->synchronizeObject(object, QpDao<T>::IgnoreTimes) == Qp::Updated;
}


template<class T>
Qp::UpdateResult QpStorage::update(QSharedPointer<T> object)
{
    beginTransaction();
    Qp::UpdateResult result = dataAccessObject<T>()->updateObject(object);
    if(result == Qp::UpdateConflict) {
        qWarning() << "Update conflict for " << T::staticMetaObject.className() << primaryKey(object);
        qFatal("Aborting");
        database().rollback();
        return Qp::UpdateConflict;
    }

    Qp::CommitResult commitResult = commitOrRollbackTransaction();
    if(commitResult == Qp::CommitSuccessful)
        return result;

    return Qp::UpdateError;
}

template<class T>
Qp::SynchronizeResult QpStorage::synchronize(QSharedPointer<T> object)
{
    return dataAccessObject<T>()->synchronizeObject(object);
}

template<class T>
bool QpStorage::remove(QSharedPointer<T> object)
{
    beginTransaction();
    dataAccessObject<T>()->removeObject(object);
    return commitOrRollbackTransaction() == Qp::CommitSuccessful;
}

template<class T>
int QpStorage::primaryKey(QSharedPointer<T> object)
{
    return Qp::Private::primaryKey(object.data());
}

template<class T>
bool QpStorage::isDeleted(QSharedPointer<T> object)
{
    return Qp::Private::isDeleted(object.data());
}

template<class T>
bool QpStorage::markAsDeleted(QSharedPointer<T> object)
{
    return dataAccessObject<T>()->markAsDeleted(object);
}

template<class T>
bool QpStorage::undelete(QSharedPointer<T> object)
{
    return dataAccessObject<T>()->undelete(object);
}

#ifndef QP_NO_TIMESTAMPS
QDateTime dateFromDouble(double value);

template<class T>
QList<QSharedPointer<T> > QpStorage::createdSince(const QDateTime &time)
{
    return Qp::castList<T>(dataAccessObject<T>()->createdSince(time));
}

template<class T>
QList<QSharedPointer<T> > QpStorage::updatedSince(const QDateTime &time)
{
    return Qp::castList<T>(dataAccessObject<T>()->updatedSince(time));
}

template<class T> QDateTime QpStorage::creationTimeInDatabase(QSharedPointer<T> object)
{
    return dateFromDouble(creationTimeInDatabase(object.data()));
}

template<class T> QDateTime QpStorage::creationTimeInInObject(QSharedPointer<T> object)
{
    return dateFromDouble(creationTimeInInObject(object.data()));
}

template<class T> QDateTime QpStorage::updateTimeInDatabase(QSharedPointer<T> object)
{
    return dateFromDouble(updateTimeInDatabase(object.data()));
}

template<class T> QDateTime QpStorage::updateTimeInObject(QSharedPointer<T> object)
{
    return dateFromDouble(updateTimeInObject(object.data()));
}
#endif

#ifndef QP_NO_LOCKS

template<class T> bool QpStorage::isLocked(QSharedPointer<T> object, IsLockedOption option)
{
    if(option == LockStateFromLastSync)
        return QpLock::isLocked(qSharedPointerCast<QObject>(object));

    QpLock lock = QpLock::lockStatus(this, qSharedPointerCast<QObject>(object));
    return (lock.status() == QpLock::LockedRemotely
            || lock.status() == QpLock::LockedLocally);
}

template<class T> QpLock QpStorage::tryLock(QSharedPointer<T> object, QHash<QString,QVariant> additionalInformation)
{
    return QpLock::tryLock(this, qSharedPointerCast<QObject>(object), additionalInformation);
}

template<class T> QpLock QpStorage::unlock(QSharedPointer<T> object)
{
    return QpLock::unlock(this, qSharedPointerCast<QObject>(object));
}

template<class T> QpLock QpStorage::lockStatus(QSharedPointer<T> object)
{
    return QpLock::lockStatus(this, qSharedPointerCast<QObject>(object));
}
#endif

#endif // QPERSISTENCE_STORAGE_H
