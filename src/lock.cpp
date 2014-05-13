#include "lock.h"

#ifndef QP_NO_LOCKS

#include "databaseschema.h"
#include "error.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqlcondition.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
#include <QSharedData>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


/**********************************************************
 *  QpLockData
 */
class QpLockData : public QSharedData {
    public:
        QpLockData();
        ~QpLockData();

        int id;
        QpError error;
        QSharedPointer<QObject> object;
        QHash<QString, QVariant> information;
        QpLock::Status status;

        static QpLock insertLock(QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation);
        static QpLock selectLock(int id, QSharedPointer<QObject> object);
        static int selectLockId(QSharedPointer<QObject> object, bool forUpdate);
        static void removeLock(int id, QSharedPointer<QObject> object);

        static bool locksEnabled;
};

typedef QHash<QSharedPointer<QObject>, QpLock> LocksHash;
QP_DEFINE_STATIC_LOCAL(LocksHash, LOCALLOCKS)

bool QpLockData::locksEnabled = false;
typedef QHash<QString, QVariant::Type> HashStringToVariantType;
QP_DEFINE_STATIC_LOCAL(HashStringToVariantType, AdditionalFields)

QpLockData::QpLockData() : QSharedData(),
    id(-1),
    status(QpLock::UnkownStatus)
{}

QpLockData::~QpLockData()
{
}

QpLock QpLockData::insertLock(QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation)
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);
    QpSqlQuery query(Qp::database());
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);

    foreach(QString field, AdditionalFields()->keys()) {
        query.addField(field, additionalInformation.value(field));
    }

    query.prepareInsert();

    if (!query.exec() || query.lastError().isValid()) {
        return QpLock(QpError(query.lastError()));
    }

    QpLock lock;
    lock.data->object = object;
    lock.data->id = query.lastInsertId().toInt();
    lock.data->status = QpLock::LockedLocally;

    foreach(QString field, AdditionalFields()->keys()) {
        lock.data->information.insert(field, additionalInformation.value(field));
    }

    query.clear();
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_LOCK, lock.data->id);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::primaryKey(object)));
    query.prepareUpdate();

    if (!query.exec() || query.lastError().isValid()) {
        return QpLock(QpError(query.lastError()));
    }

    LOCALLOCKS()->insert(object, lock);
    return lock;
}

QpLock QpLockData::selectLock(int id, QSharedPointer<QObject> object)
{
    QpSqlQuery query(Qp::database());
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    foreach(QString field, AdditionalFields()->keys()) {
        query.addField(field);
    }
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           id));
    query.prepareSelect();

    if (!query.exec()
        || !query.first()
        || query.lastError().isValid()) {
        return QpLock(QpError(query.lastError()));
    }

    QpLock lock;
    lock.data->object = object;
    lock.data->id = query.value(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY).toInt();
    lock.data->status = QpLock::LockedRemotely;

    foreach(QString field, AdditionalFields()->keys()) {
        QVariant value = query.value(field);
        value.convert(static_cast<int>(AdditionalFields()->value(field)));
        lock.data->information.insert(field, value);
    }

    return lock;
}

int QpLockData::selectLockId(QSharedPointer<QObject> object, bool forUpdate)
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);

    // Check if there already is a different lock
    QpSqlQuery query(Qp::database());
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_LOCK);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::primaryKey(object)));
    query.setForUpdate(forUpdate);
    query.prepareSelect();

    if (!query.exec()
        || !query.first()
        || query.lastError().isValid()) {
        Qp::Private::setLastError(QpError(query.lastError()));
        return 0;
    }

    return query.value(0).toInt();
}

void QpLockData::removeLock(int id, QSharedPointer<QObject> object)
{
    LOCALLOCKS()->remove(object);

    QpSqlQuery query(Qp::database());
    query.clear();
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           id));
    query.prepareDelete();

    if (!query.exec()
        || query.lastError().isValid()) {
        Qp::Private::setLastError(QpError(query.lastError()));
        return;
    }
}
#endif

/**********************************************************
 *  QpLock
 */
QpLock::QpLock()
#ifndef QP_NO_LOCKS
    : data(new QpLockData)
    #endif
{
}

#ifndef QP_NO_LOCKS

QpLock::QpLock(const QpError &error) : data(new QpLockData)
{
    data->error = error;
    data->status = DatabaseError;
    Qp::Private::setLastError(error);
}

QpLock::QpLock(const QpLock &rhs) : data(rhs.data)
{
}

QpLock &QpLock::operator=(const QpLock &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QpLock::~QpLock()
{
}

bool QpLock::isLocksEnabled()
{
    return QpLockData::locksEnabled;
}

void QpLock::addAdditionalInformationField(const QString &name, QVariant::Type type)
{
    AdditionalFields()->insert(name, type);
}

QHash<QString, QVariant::Type> QpLock::additionalInformationFields()
{
    return *AdditionalFields();
}

void QpLock::enableLocks()
{
    QpLockData::locksEnabled = true;
}

QpError QpLock::error() const
{
    return data->error;
}

QSharedPointer<QObject> QpLock::object() const
{
    return data->object;
}

QVariant QpLock::additionalInformation(const QString &name) const
{
    return data->information.value(name);
}

QpLock QpLock::isLocked(QSharedPointer<QObject> object)
{
    if(!Qp::beginTransaction()) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("TRANSACTION failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    QpLock localLock = LOCALLOCKS()->value(object);

    int lockId = QpLockData::selectLockId(object, false);
    QpLock lock;

    if (lockId == 0) {
        // There is no lock
        lock.data->object = object;

        if(localLock.status() == LockedLocally) {
            // Someone else unlocked my lock!
            LOCALLOCKS()->remove(object);
            lock.data->status = QpLock::UnlockedRemotely;
        }
        else {
            lock.data->status = QpLock::NotLocked;
        }
    }
    else {
        if(localLock.status() == LockedLocally) {
            if(localLock.data->id == lockId) {
                // This is my lock
                lock = localLock;
            }
            else {
                // Someone locked my object!
                LOCALLOCKS()->remove(object);
                lock = QpLockData::selectLock(lockId, object);
            }
        }
        else {
            // There is a (remote) lock
            lock = QpLockData::selectLock(lockId, object);
        }
    }

    if(Qp::commitOrRollbackTransaction() != Qp::CommitSuccessful) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("COMMIT failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    return lock;

}

QpLock QpLock::tryLock(QSharedPointer<QObject> object, QHash<QString, QVariant> additionalInformation)
{
    if(!Qp::beginTransaction()) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("TRANSACTION failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    QpLock localLock = LOCALLOCKS()->value(object);

    // Check if there already is a different lock
    int lockId = QpLockData::selectLockId(object, true);
    QpLock lock;

    if (lockId == 0) {
        // There is no lock

        if(localLock.status() == LockedLocally) {
            // Someone else unlocked my lock!
            LOCALLOCKS()->remove(object);
        }

        lock = QpLockData::insertLock(object, additionalInformation);
    }
    else {
        // There is a lock

        if(localLock.status() == LockedLocally) {
            if(localLock.data->id == lockId) {
                // This is my lock
                lock = localLock;
            }
            else {
                // Someone locked my object!
                LOCALLOCKS()->remove(object);
                lock = QpLockData::selectLock(lockId, object);
            }
        }
        else {
            // There is a (remote) lock
            lock = QpLockData::selectLock(lockId, object);
        }
    }

    if(Qp::commitOrRollbackTransaction() != Qp::CommitSuccessful) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("COMMIT failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    return lock;
}

QpLock QpLock::unlock(QSharedPointer<QObject> object)
{
    if(!Qp::beginTransaction()) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("TRANSACTION failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    // Local lock
    QpLock localLock = LOCALLOCKS()->value(object);

    // Remote lock
    int lockId = QpLockData::selectLockId(object, true);
    QpLock lock;

    if (lockId != 0) {
        // There is a lock
        lock = QpLockData::selectLock(lockId, object);

        if(localLock.status() == LockedLocally) {
            if(lockId != localLock.data->id) {
                // Someone else locked my object! AAAH!!
                lock.data->status = UnlockedRemoteLock;
            }
            else {
                lock.data->status = UnlockedLocalLock;
            }
        }
        else {
            lock.data->status = UnlockedRemoteLock;
        }

        QpLockData::removeLock(lockId, object);
    }
    else {
        // There is no lock

        if(localLock.status() == LockedLocally) {
            // Someone else unlocked my lock! AAAH!!
            lock.data->status = UnlockedRemotely;
            LOCALLOCKS()->remove(object);
        }
        else {
            lock.data->status = NotLocked;
        }
    }

    if(Qp::commitOrRollbackTransaction() != Qp::CommitSuccessful) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("COMMIT failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    lock.data->object = object;
    return lock;
}

QpLock::Status QpLock::status() const
{
    return data->status;
}

#endif
