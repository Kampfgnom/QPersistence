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
class QpAbstractErrorHandler;
class QpError;
class QpTransactionsHelper;

class QpStorageData;
class QpStorage : public QObject
{
    Q_OBJECT
public:
    static QpStorage *defaultStorage();

    explicit QpStorage(QObject *parent = 0);
    ~QpStorage();

    QSqlDatabase database() const;
    void setDatabase(const QSqlDatabase &database);
    void setSqlDebugEnabled(bool enable);
    bool adjustDatabaseSchema();
    bool createCleanSchema();

    QpError lastError() const;
    void setLastError(const QpError &error);
    void setLastError(const QSqlQuery &query);
    void addErrorHandler(QpAbstractErrorHandler *handler);
    void clearErrorHandlers();

    bool beginTransaction();
    bool commitOrRollbackTransaction();
    bool rollbackTransaction();

    void startBulkDatabaseQueries();
    void commitBulkDatabaseQueries();

    void resetAllLastKnownSynchronizations();

    QList<QpDaoBase *> dataAccessObjects();
    QpDaoBase *dataAccessObject(const QMetaObject metaObject) const;
    QpDaoBase *dataAccessObject(const QString &className) const;
    QpDaoBase *dataAccessObject(int userType) const;
    Qp::SynchronizeResult synchronize(QSharedPointer<QObject> object, QpDaoBase::SynchronizeMode mode);
    Qp::UpdateResult update(QSharedPointer<QObject> object);
    bool incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName);
    bool remove(QSharedPointer<QObject> object);
    int primaryKey(QSharedPointer<QObject> object);
    bool isDeleted(QSharedPointer<QObject> object);
    bool markAsDeleted(QSharedPointer<QObject> object);
    bool undelete(QSharedPointer<QObject> object);
    QpDaoBase *dataAccessObject(QSharedPointer<QObject> object) const;

    QpSqlDataAccessObjectHelper *sqlDataAccessObjectHelper() const;
    void enableStorageFrom(QObject *object);
    static QpStorage *forObject(const QObject *object);
    static QpStorage *forObject(QSharedPointer<QObject> object);

    int revisionInDatabase(QObject *object);
    int revisionInObject(QObject *object);

    template<class T, class ... Superclasses> void registerClass();

    template<class T> QpDao<T> *dataAccessObject();
    template<class T> QpDaoBase *dataAccessObject(QSharedPointer<T> object) const;
    template<class T> int primaryKey(QSharedPointer<T> object);
    template<class T> QSharedPointer<T> read(int id);
    template<class T> QList<QSharedPointer<T> > readAll(const QpSqlCondition &condition = QpSqlCondition());
    template<class T> int count(const QpSqlCondition &condition = QpSqlCondition());
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
    template<class T> QList<QSharedPointer<T> > createdSince(const QDateTime &time);
    template<class T> QList<QSharedPointer<T> > updatedSince(const QDateTime &time);
    template<class T> QDateTime creationTime(QSharedPointer<T> object);
    template<class T> QDateTime creationTimeInDatabase(QSharedPointer<T> object);
    template<class T> QDateTime creationTimeInInObject(QSharedPointer<T> object);
    template<class T> QDateTime updateTimeInDatabase(QSharedPointer<T> object);
    template<class T> QDateTime updateTimeInObject(QSharedPointer<T> object);
    double creationTime(QObject *object);
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

private:
    void registerDataAccessObject(QpDaoBase *dao, const QMetaObject *metaObject);
    QExplicitlySharedDataPointer<QpStorageData> data;
};

template <class T>
bool QpStorage::incrementNumericColumn(QSharedPointer<T> object, const QString &fieldName)
{
    return incrementNumericColumn(qSharedPointerCast<QObject>(object), fieldName);
}

template <class T>
Qp::SynchronizeResult QpStorage::synchronize(QSharedPointer<T> object)
{
    return synchronize(qSharedPointerCast<QObject>(object));
}

template <class T>
bool QpStorage::isDeleted(QSharedPointer<T> object)
{
    return isDeleted(qSharedPointerCast<QObject>(object));
}

template <class T>
bool QpStorage::markAsDeleted(QSharedPointer<T> object)
{
    return markAsDeleted(qSharedPointerCast<QObject>(object));
}

template <class T>
bool QpStorage::remove(QSharedPointer<T> object)
{
    return remove(qSharedPointerCast<QObject>(object));
}

template <class T>
Qp::UpdateResult QpStorage::update(QSharedPointer<T> object)
{
    return update(qSharedPointerCast<QObject>(object));
}

template <class T>
int QpStorage::primaryKey(QSharedPointer<T> object)
{
    return primaryKey(qSharedPointerCast<QObject>(object));
}

template <class T>
QpDaoBase *QpStorage::dataAccessObject(QSharedPointer<T> object) const
{
    return dataAccessObject(qSharedPointerCast<QObject>(object));
}

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

template<typename ... Args> void unpackTemplateParameters(Args ...) {
}

template<class T, class ... Superclasses>
void QpStorage::registerClass()
{
    QpDao<T> *dao = new QpDao<T>(this);
    registerDataAccessObject(dao, &T::staticMetaObject);

    registerMetaType<T>();

    // http://stackoverflow.com/questions/12515616/expression-contains-unexpanded-parameter-packs/12515637#12515637
    unpackTemplateParameters((registerMetaType<Superclasses>(), 0) ...);
}

template<class T> QSharedPointer<T> QpStorage::read(int id)
{
    return dataAccessObject<T>()->read(id);
}

template<class T>
QList<QSharedPointer<T> > QpStorage::readAll(const QpSqlCondition &condition)
{
    return dataAccessObject<T>()->readAllObjects(-1, -1, QpSqlCondition::notDeletedAnd(condition));
}

template<class T>
int QpStorage::count(const QpSqlCondition &condition)
{
    return dataAccessObject<T>()->count(QpSqlCondition::notDeletedAnd(condition));
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

template<class T> QDateTime QpStorage::creationTime(QSharedPointer<T> object)
{
    return dateFromDouble(creationTime(object.data()));
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
    if (option == LockStateFromLastSync)
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
