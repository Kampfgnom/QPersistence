#include "lock.h"
#include <QSharedData>

#include "databaseschema.h"
#include "error.h"
#include "qpersistence.h"
#include "sqlcondition.h"
#include "sqlquery.h"

#include <QSqlError>


/**********************************************************
 *  QpLockData
 */
class QpLockData : public QSharedData {
public:
    QpLockData();
    ~QpLockData();

    int id;
    QpError error;
    QpLock::Status status;
    QSharedPointer<QObject> object;
    QHash<QString, QVariant> information;

    static QpLock insertLock(QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation);
    static QpLock selectLock(int id, QSharedPointer<QObject> object);
    static int selectLockId(QSharedPointer<QObject> object, bool forUpdate);
    static void removeLock(int id, QSharedPointer<QObject> object);

    static bool locksEnabled;
    static QHash<QString, QVariant::Type> additionalFields;
};

typedef QHash<QSharedPointer<QObject>, QpLock> LocksHash;
Q_GLOBAL_STATIC(LocksHash, LOCALLOCKS)
#undef COMMA

bool QpLockData::locksEnabled = false;
QHash<QString, QVariant::Type> QpLockData::additionalFields;

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

    foreach(QString field, QpLockData::additionalFields.keys()) {
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

    foreach(QString field, QpLockData::additionalFields.keys()) {
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
    foreach(QString field, QpLockData::additionalFields.keys()) {
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

    foreach(QString field, QpLockData::additionalFields.keys()) {
        QVariant value = query.value(field);
        value.convert(QpLockData::additionalFields.value(field));
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
    QpMetaObject metaObject = QpMetaObject::forObject(object);

    QpSqlQuery query(Qp::database());
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_LOCK, 0);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::primaryKey(object)));
    query.prepareUpdate();

    if (!query.exec()
            || query.lastError().isValid()) {
        Qp::Private::setLastError(QpError(query.lastError()));
        return;
    }

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

    if(LOCALLOCKS()->contains(object))
        LOCALLOCKS()->remove(object);
}

/**********************************************************
 *  QpLock
 */
QpLock::QpLock() : data(new QpLockData)
{
}

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
    QpLockData::additionalFields.insert(name, type);
}

QHash<QString, QVariant::Type> QpLock::additionalInformationFields()
{
    return QpLockData::additionalFields;
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
    if(LOCALLOCKS()->contains(object))
        return LOCALLOCKS()->value(object);

    if(!Qp::beginTransaction()) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("TRANSACTION failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    int lockId = QpLockData::selectLockId(object, false);
    QpLock lock;

    if (lockId == 0) {
        // There is no lock
        lock.data->status = QpLock::Unlocked;
        lock.data->object = object;
    }
    else {
        // There is a (remote) lock
        lock = QpLockData::selectLock(lockId, object);
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
    if(LOCALLOCKS()->contains(object))
        return LOCALLOCKS()->value(object);

    if(!Qp::beginTransaction()) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("TRANSACTION failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    // Check if there already is a different lock
    int lockId = QpLockData::selectLockId(object, true);
    QpLock lock;

    if (lockId == 0) {
        // There is no other lock
        lock = QpLockData::insertLock(object, additionalInformation);
    }
    else {
        // There is already a lock
        lock = QpLockData::selectLock(lockId, object);
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

    // Check if there already is a different lock
    int lockId = QpLockData::selectLockId(object, true);
    QpLock lock;
    if (lockId != 0) {
        // There is a lock
        lock = QpLockData::selectLock(lockId, object);
        QpLock localLock = LOCALLOCKS()->value(object);

        if(localLock.status() == LockedLocally) {
            Q_ASSERT(lock.data->id == localLock.data->id);
        }

        QpLockData::removeLock(lockId, object);
    }

    if(Qp::commitOrRollbackTransaction() != Qp::CommitSuccessful) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("COMMIT failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    lock.data->object = object;
    lock.data->status = Unlocked;
    return lock;
}

QpLock::Status QpLock::status() const
{
    return data->status;
}
