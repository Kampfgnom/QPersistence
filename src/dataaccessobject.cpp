#include "dataaccessobject.h"

#include "cache.h"
#include "databaseschema.h"
#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqldataaccessobjecthelper.h"
#include "sqlquery.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
#include <QSqlRecord>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpDaoBaseData : public QSharedData
{
    public:
        QpDaoBaseData() :
            QSharedData(),
            lastSync(0.0)
        {
        }

        QpStorage *storage;
        QpMetaObject metaObject;
        mutable QpError lastError;
        mutable QpCache cache;
        double lastSync;
};


QpDaoBase::QpDaoBase(const QMetaObject &metaObject,
                     QpStorage *parent) :
    QObject(parent),
    data(new QpDaoBaseData)
{
    data->storage = parent;
    data->metaObject = QpMetaObject::registerMetaObject(metaObject);
}

QpDaoBase::~QpDaoBase()
{
}

QpError QpDaoBase::lastError() const
{
    return data->lastError;
}

QpCache QpDaoBase::cache() const
{
    return data->cache;
}

void QpDaoBase::setLastError(const QpError &error) const
{
    data->lastError = error;
    data->storage->setLastError(error);
}

void QpDaoBase::resetLastError() const
{
    setLastError(QpError());
}

Qp::SynchronizeResult QpDaoBase::sync(QSharedPointer<QObject> object)
{
    QObject *obj = object.data();
    int id = data->storage->primaryKey(object);
    data->storage->setSqlDebugEnabled(true);
    if (!data->storage->sqlDataAccessObjectHelper()->readObject(data->metaObject, id, obj)) {
        QpError error = data->storage->lastError();
        if(error.isValid()) {
            setLastError(error);
            return Qp::Error;
        }

        data->cache.remove(data->storage->primaryKey(object));
        emit objectRemoved(object);

        return Qp::Removed;
    }

    foreach(QpMetaProperty relation, QpMetaObject::forObject(object).relationProperties()) {
        QpRelationResolver::readRelationFromDatabase(relation, obj);
    }

    data->storage->setSqlDebugEnabled(false);
    if(Qp::Private::isDeleted(object.data())) {
        emit objectMarkedAsDeleted(object);
        return Qp::Deleted;
    }

    emit objectUpdated(object);
    return Qp::Updated;
}

QpMetaObject QpDaoBase::qpMetaObject() const
{
    return data->metaObject;
}

int QpDaoBase::count() const
{
    return data->storage->sqlDataAccessObjectHelper()->count(data->metaObject);
}

QList<int> QpDaoBase::allKeys(int skip, int count) const
{
    QList<int> result = data->storage->sqlDataAccessObjectHelper()->allKeys(data->metaObject, skip, count);

    if (data->storage->lastError().isValid())
        setLastError(data->storage->lastError());

    return result;
}

QList<QSharedPointer<QObject> > QpDaoBase::readAllObjects(int skip, int count, const QpSqlCondition &condition) const
{
    QpSqlQuery query = data->storage->sqlDataAccessObjectHelper()->readAllObjects(data->metaObject, skip, count, condition);

    if (data->storage->lastError().isValid()) {
        setLastError(data->storage->lastError());
        return QList<QSharedPointer<QObject> >();
    }

    QList<QSharedPointer<QObject> > result;
    result.reserve(count);
    QSqlRecord record = query.record();
    int index = record.indexOf(QLatin1String(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY));
    int updateTimeRecordIndex = -1;

#ifndef QP_NO_TIMESTAMPS
    updateTimeRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME);
#endif

    while(query.next()) {
        int key = query.value(index).toInt();

        QSharedPointer<QObject> currentObject;
        if (data->cache.contains(key)) {
            currentObject = data->cache.get(key);
        }

        if(!currentObject) {
            QObject *object = createInstance();
            data->storage->enableStorageFrom(object);
            currentObject = data->cache.insert(key, object);
            data->storage->sqlDataAccessObjectHelper()->readQueryIntoObject(query, record, object, index, updateTimeRecordIndex);
            Qp::Private::enableSharedFromThis(currentObject);
        }

        result.append(currentObject);
    }

    if (data->storage->lastError().isValid()) {
        setLastError(data->storage->lastError());
        return QList<QSharedPointer<QObject> >();
    }

    return result;
}

QSharedPointer<QObject> QpDaoBase::readObject(int id) const
{
    QSharedPointer<QObject> p = data->cache.get(id);

    if (p)
        return p;

    QObject *object = createInstance();
    data->storage->enableStorageFrom(object);
    QSharedPointer<QObject> obj = data->cache.insert(id, object);
    Qp::Private::enableSharedFromThis(obj);

    if (!data->storage->sqlDataAccessObjectHelper()->readObject(data->metaObject, id, object)) {
        QpError error = data->storage->lastError();
        if(error.isValid())
            setLastError(error);

        data->cache.remove(id);
        delete object;
        return QSharedPointer<QObject>();
    }

    return obj;
}

QSharedPointer<QObject> QpDaoBase::createObject()
{
    QObject *object = createInstance();
    data->storage->enableStorageFrom(object);

    if (!data->storage->sqlDataAccessObjectHelper()->insertObject(data->metaObject, object)) {
        setLastError(data->storage->lastError());
        return QSharedPointer<QObject>();
    }
    QSharedPointer<QObject> obj = data->cache.insert(Qp::Private::primaryKey(object), object);
    Qp::Private::enableSharedFromThis(obj);

    emit objectCreated(obj);
    return obj;
}

#ifndef QP_NO_TIMESTAMPS
Q_DECL_CONSTEXPR static inline bool qpFuzzyCompare(double p1, double p2) Q_REQUIRED_RESULT;
Q_DECL_CONSTEXPR static inline bool qpFuzzyCompare(double p1, double p2)
{
    return (qAbs(p1 - p2) * 10000000000000000. <= qMin(qAbs(p1), qAbs(p2)));
}
#endif

Qp::UpdateResult QpDaoBase::updateObject(QSharedPointer<QObject> object)
{
#ifndef QP_NO_TIMESTAMPS
    double databaseTime = data->storage->updateTimeInDatabase(object.data());
    double objectTime = data->storage->updateTimeInObject(object.data());

    if(databaseTime > objectTime)
        return Qp::UpdateConflict;

    Q_ASSERT(qpFuzzyCompare(databaseTime, objectTime));
#endif

    if (!data->storage->sqlDataAccessObjectHelper()->updateObject(data->metaObject, object.data())) {
        setLastError(data->storage->lastError());
        return Qp::UpdateError;
    }

    emit objectUpdated(object);
    return Qp::UpdateSuccess;
}

bool QpDaoBase::removeObject(QSharedPointer<QObject> object)
{
    if (!data->storage->sqlDataAccessObjectHelper()->removeObject(data->metaObject, object.data())) {
        setLastError(data->storage->lastError());
        return false;
    }

    data->cache.remove(data->storage->primaryKey(object));
    emit objectRemoved(object);
    return true;
}

bool QpDaoBase::markAsDeleted(QSharedPointer<QObject> object)
{
    Qp::Private::markAsDeleted(object.data());
    Qp::UpdateResult result = updateObject(object);
    if(result != Qp::UpdateSuccess)
        return false;

    emit objectMarkedAsDeleted(object);
    return true;
}

bool QpDaoBase::undelete(QSharedPointer<QObject> object)
{
    Qp::Private::undelete(object.data());
    Qp::UpdateResult result = updateObject(object);
    if(result != Qp::UpdateSuccess)
        return false;

    emit objectUndeleted(object);
    return true;
}


Qp::SynchronizeResult QpDaoBase::synchronizeObject(QSharedPointer<QObject> object, SynchronizeMode mode)
{
    if(mode == IgnoreTimes)
        return sync(object);

#ifndef QP_NO_TIMESTAMPS
    QObject *obj = object.data();
    double localTime = data->storage->updateTimeInObject(obj);
    double remoteTime = data->storage->updateTimeInDatabase(obj);

    if(qpFuzzyCompare(localTime, remoteTime))
        return Qp::Unchanged;
#endif

    return sync(object);
}

bool QpDaoBase::synchronizeAllObjects()
{
#ifndef QP_NO_TIMESTAMPS
    double currentTime = data->storage->databaseTimeInternal();

    if(data->lastSync > 0.0) {
        QHash<QObject *, bool> createdObjects;
        foreach(QSharedPointer<QObject> object, createdSince(data->lastSync)) {
            emit objectCreated(object);
            createdObjects.insert(object.data(), true);
        }
        foreach(QSharedPointer<QObject> object, updatedSince(data->lastSync)) {
            if(createdObjects.value(object.data(), false))
                continue;

            sync(object);
        }
    }

    data->lastSync = currentTime;
    if(lastError().isValid())
        return false;
#endif

    return true;
}

bool QpDaoBase::incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName)
{
    if (!data->storage->sqlDataAccessObjectHelper()->incrementNumericColumn(object.data(), fieldName)) {
        setLastError(data->storage->lastError());
        return false;
    }

    return true;
}

#ifndef QP_NO_TIMESTAMPS
QList<QSharedPointer<QObject> > QpDaoBase::createdSince(const QDateTime &time)
{
    return createdSince(time.toString("yyyyMMddHHmmss.zzz").toDouble());
}

QList<QSharedPointer<QObject> > QpDaoBase::createdSince(double time)
{
    return readAllObjects(-1,-1, QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME,
                                                QpSqlCondition::GreaterThanOrEqualTo,
                                                time));
}

QList<QSharedPointer<QObject> > QpDaoBase::updatedSince(const QDateTime &time)
{
    return updatedSince(time.toString("yyyyMMddHHmmss.zzz").toDouble());
}

QList<QSharedPointer<QObject> > QpDaoBase::updatedSince(double time)
{
    return readAllObjects(-1,-1, QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME,
                                                QpSqlCondition::GreaterThanOrEqualTo,
                                                time));
}
#endif
