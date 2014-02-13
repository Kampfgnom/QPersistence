#include "lock.h"
#include <QSharedData>

#include "databaseschema.h"
#include "error.h"
#include "qpersistence.h"
#include "sqlcondition.h"
#include "sqlquery.h"

#include <QSqlError>

class QpLockData : public QSharedData {
public:
    QpLockData() : QSharedData(),
        id(-1),
        status(QpLock::UnkownStatus)
    {}

    ~QpLockData()
    {
        if(object)
            locks.remove(object);
    }

    int id;
    QpError error;
    QpLock::Status status;
    QSharedPointer<QObject> object;

    static bool locksEnabled;
    static QHash<QSharedPointer<QObject>, QpLock> locks;
};

bool QpLockData::locksEnabled = false;
QHash<QSharedPointer<QObject>, QpLock> QpLockData::locks;

QpLock::QpLock() : data(new QpLockData)
{
}

QpLock::QpLock(const QpError &error) : data(new QpLockData)
{
    data->error = error;
    data->status = DatabaseError;
}

int QpLock::id() const
{
    return data->id;
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

QpLock QpLock::tryLock(QSharedPointer<QObject> object)
{
    if(QpLockData::locks.contains(object))
        return QpLockData::locks.value(object);

    if(!Qp::beginTransaction()) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("TRANSACTION failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    QpMetaObject metaObject = QpMetaObject::forClassName(object->metaObject()->className());

    // Check if there already is a different lock
    QpSqlQuery query(Qp::database());
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_LOCK);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::primaryKey(object)));
    query.setForUpdate(true);
    query.prepareSelect();

    if (!query.exec()
            || !query.first()
            || query.lastError().isValid()) {
        return QpLock(QpError(query.lastError()));
    }

    int lockId = query.value(0).toInt();
    QpLock lock;

    if (lockId == 0) {
        // There is no other lock
        lock = insertLock(object);
    }
    else {
        // There is already a lock
        lock = selectLock(lockId, object);
    }

    if(Qp::commitOrRollbackTransaction() != Qp::CommitSuccessful) {
        QpError error = Qp::lastError();
        if(!error.isValid())
            error = QpError("COMMIT failed while locking", QpError::TransactionError);
        return QpLock(error);
    }

    return lock;
}

QpLock::Status QpLock::status() const
{
    return data->status;
}

QpLock QpLock::insertLock(QSharedPointer<QObject> object)
{
    QpMetaObject metaObject = QpMetaObject::forClassName(object->metaObject()->className());
    QpSqlQuery query(Qp::database());
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    query.prepareInsert();

    if (!query.exec() || query.lastError().isValid()) {
        return QpLock(QpError(query.lastError()));
    }

    QpLock lock;
    lock.data->object = object;
    lock.data->id = query.lastInsertId().toInt();
    lock.data->status = LockedLocally;

    query.clear();
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_LOCK, lock.id());
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::primaryKey(object)));
    query.prepareUpdate();

    if (!query.exec() || query.lastError().isValid()) {
        return QpLock(QpError(query.lastError()));
    }

    return lock;
}

QpLock QpLock::selectLock(int id, QSharedPointer<QObject> object)
{
    QpSqlQuery query(Qp::database());
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
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
    lock.data->id = query.value(0).toInt();
    lock.data->status = LockedRemotely;
    return lock;
}
