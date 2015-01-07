#include "lock.h"

#ifndef QP_NO_LOCKS

#include "databaseschema.h"
#include "error.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqlcondition.h"
#include "sqlquery.h"
#include "storage.h"
#include "transactionshelper.h"

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

    static QpLock insertLock(QpStorage *storage, QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation);
    static QpLock selectLock(QpStorage *storage, int id, QSharedPointer<QObject> object);
    static int selectLockId(QpStorage *storage, QSharedPointer<QObject> object, bool forUpdate);
    static void removeLock(QpStorage *storage, int id, QSharedPointer<QObject> object);
};

typedef QHash<QSharedPointer<QObject>, QpLock> LocksHash;
QP_DEFINE_STATIC_LOCAL(LocksHash, LOCALLOCKS)

QpLockData::QpLockData() :
    QSharedData(),
    id(-1),
    status(QpLock::UnkownStatus)
{
}

QpLockData::~QpLockData()
{
}

QpLock QpLockData::insertLock(QpStorage *storage, QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation)
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);
    QpSqlQuery query(storage->database());
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);

    foreach (QString field, storage->additionalLockInformationFields().keys()) {
        query.addField(field, additionalInformation.value(field));
    }

    query.prepareInsert();

    if (!query.exec() || query.lastError().isValid()) {
        QpLock lock = QpLock(QpError(query.lastError()));
        storage->setLastError(lock.error());
        return lock;
    }

    QpLock lock;
    lock.data->object = object;
    lock.data->id = query.lastInsertId().toInt();
    lock.data->status = QpLock::LockedLocally;

    foreach (QString field, storage->additionalLockInformationFields().keys()) {
        lock.data->information.insert(field, additionalInformation.value(field));
    }

    query.clear();
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_LOCK, lock.data->id);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object.data())));
    query.prepareUpdate();

    if (!query.exec() || query.lastError().isValid()) {
        lock = QpLock(QpError(query.lastError()));
        storage->setLastError(lock.error());
        return lock;
    }

    LOCALLOCKS()->insert(object, lock);
    return lock;
}

QpLock QpLockData::selectLock(QpStorage *storage, int id, QSharedPointer<QObject> object)
{
    QpSqlQuery query(storage->database());
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    foreach (QString field, storage->additionalLockInformationFields().keys()) {
        query.addField(field);
    }
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           id));
    query.prepareSelect();

    if (!query.exec()
        || !query.first()
        || query.lastError().isValid()) {
        QpLock lock = QpLock(QpError(query.lastError()));
        storage->setLastError(lock.error());
        return lock;
    }

    QpLock lock;
    lock.data->object = object;
    lock.data->id = query.value(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY).toInt();
    lock.data->status = QpLock::LockedRemotely;

    foreach (QString field, storage->additionalLockInformationFields().keys()) {
        QVariant value = query.value(field);
        value.convert(static_cast<int>(storage->additionalLockInformationFields().value(field)));
        lock.data->information.insert(field, value);
    }

    return lock;
}

int QpLockData::selectLockId(QpStorage *storage, QSharedPointer<QObject> object, bool forUpdate)
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);

    // Check if there already is a different lock
    QpSqlQuery query(storage->database());
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_LOCK);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object.data())));
    query.setForUpdate(forUpdate);
    query.prepareSelect();

    if (!query.exec()
        || query.lastError().isValid()) {
        storage->setLastError(QpError(query.lastError()));
        return -1;
    }

    if (!query.first())
        return 0;

    return query.value(0).toInt();
}

void QpLockData::removeLock(QpStorage *storage, int id, QSharedPointer<QObject> object)
{
    LOCALLOCKS()->remove(object);

    QpSqlQuery query(storage->database());
    query.clear();
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           id));
    query.prepareDelete();

    if (!query.exec()
        || query.lastError().isValid()) {
        storage->setLastError(QpError(query.lastError()));
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

bool QpLock::isLocked(QSharedPointer<QObject> object)
{
    bool ok = false;
    int lockId = object->property(QpDatabaseSchema::COLUMN_LOCK).toInt(&ok);
    return ok && lockId > 0;
}

QpLock::QpLock(const QpError &error) :
    data(new QpLockData)
{
    data->error = error;
    data->status = DatabaseError;
}

QpLock::QpLock(const QpLock &rhs) :
    data(rhs.data)
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

QpLock QpLock::lockStatus(QpStorage *storage, QSharedPointer<QObject> object)
{
    if (!storage->beginTransaction())
        return QpLock(storage->lastError());

    QpLock localLock = LOCALLOCKS()->value(object);

    int lockId = QpLockData::selectLockId(storage, object, false);
    QpLock lock;

    if (lockId == 0) {
        // There is no lock
        lock.data->object = object;

        if (localLock.status() == LockedLocally) {
            // Someone else unlocked my lock!
            LOCALLOCKS()->remove(object);
            lock.data->status = QpLock::UnlockedRemotely;
        }
        else {
            lock.data->status = QpLock::NotLocked;
        }
    }
    else {
        if (localLock.status() == LockedLocally) {
            if (localLock.data->id == lockId) {
                // This is my lock
                lock = localLock;
            }
            else {
                // Someone locked my object!
                LOCALLOCKS()->remove(object);
                lock = QpLockData::selectLock(storage, lockId, object);
            }
        }
        else {
            // There is a (remote) lock
            lock = QpLockData::selectLock(storage, lockId, object);
        }
    }

    if (!storage->commitOrRollbackTransaction())
        return QpLock(storage->lastError());

    return lock;

}

QpLock QpLock::tryLock(QpStorage *storage, QSharedPointer<QObject> object, QHash<QString, QVariant> additionalInformation)
{
    if (!storage->beginTransaction())
        return QpLock(storage->lastError());

    QpLock localLock = LOCALLOCKS()->value(object);

    // Check if there already is a different lock
    int lockId = QpLockData::selectLockId(storage, object, true);
    QpLock lock;

    if (lockId == 0) {
        // There is no lock

        if (localLock.status() == LockedLocally) {
            // Someone else unlocked my lock!
            LOCALLOCKS()->remove(object);
        }

        lock = QpLockData::insertLock(storage, object, additionalInformation);
    }
    else {
        // There is a lock

        if (localLock.status() == LockedLocally) {
            if (localLock.data->id == lockId) {
                // This is my lock
                lock = localLock;
            }
            else {
                // Someone locked my object!
                LOCALLOCKS()->remove(object);
                lock = QpLockData::selectLock(storage, lockId, object);
            }
        }
        else {
            // There is a (remote) lock
            lock = QpLockData::selectLock(storage, lockId, object);
        }
    }

    if (!storage->commitOrRollbackTransaction())
        return QpLock(storage->lastError());

    return lock;
}

QpLock QpLock::unlock(QpStorage *storage, QSharedPointer<QObject> object)
{
    if (!storage->beginTransaction())
        return QpLock(storage->lastError());

    // Local lock
    QpLock localLock = LOCALLOCKS()->value(object);

    // Remote lock
    int lockId = QpLockData::selectLockId(storage, object, true);
    QpLock lock;

    if (lockId != 0) {
        // There is a lock
        lock = QpLockData::selectLock(storage, lockId, object);

        if (localLock.status() == LockedLocally) {
            if (lockId != localLock.data->id) {
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

        QpLockData::removeLock(storage, lockId, object);
    }
    else {
        // There is no lock

        if (localLock.status() == LockedLocally) {
            // Someone else unlocked my lock! AAAH!!
            lock.data->status = UnlockedRemotely;
            LOCALLOCKS()->remove(object);
        }
        else {
            lock.data->status = NotLocked;
        }
    }

    if (!storage->commitOrRollbackTransaction())
        return QpLock(storage->lastError());

    lock.data->object = object;
    return lock;
}

QpLock::Status QpLock::status() const
{
    return data->status;
}

#endif
