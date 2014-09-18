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
            lastSynchronizedCreatedId(0),
            lastSynchronizedRevision(0)
        {
        }

        QpStorage *storage;
        QpMetaObject metaObject;
        mutable QpError lastError;
        mutable QpCache cache;
        int lastSynchronizedCreatedId;
        int lastSynchronizedRevision;
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

void QpDaoBase::resetLastKnownSynchronization()
{
    data->storage->database().transaction();
    data->lastSynchronizedCreatedId = data->storage->sqlDataAccessObjectHelper()->maxPrimaryKey(data->metaObject);
    data->lastSynchronizedRevision = data->storage->sqlDataAccessObjectHelper()->latestRevision(data->metaObject);
    data->storage->database().commit();
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

int QpDaoBase::count(const QpSqlCondition &condition) const
{
    return data->storage->sqlDataAccessObjectHelper()->count(data->metaObject, condition);
}

QList<int> QpDaoBase::allKeys(int skip, int count) const
{
    QList<int> result = data->storage->sqlDataAccessObjectHelper()->allKeys(data->metaObject, skip, count);

    if (data->storage->lastError().isValid())
        setLastError(data->storage->lastError());

    return result;
}

QList<QSharedPointer<QObject> > QpDaoBase::readAllObjects(int skip, int count, const QpSqlCondition &condition, QList<QpSqlQuery::OrderField> orders) const
{
    QpSqlQuery query = data->storage->sqlDataAccessObjectHelper()->readAllObjects(data->metaObject, skip, count, condition, orders);
    return readAllObjects(query);
}

QList<QSharedPointer<QObject> > QpDaoBase::readObjectsUpdatedAfterRevision(int revision) const
{
    QpSqlQuery query = data->storage->sqlDataAccessObjectHelper()->readObjectsUpdatedAfterRevision(data->metaObject, revision);
    return readAllObjects(query);
}

QList<QSharedPointer<QObject> > QpDaoBase::readAllObjects(QpSqlQuery &query) const
{
    if (data->storage->lastError().isValid()) {
        return QList<QSharedPointer<QObject> >();
    }

    QList<QSharedPointer<QObject> > result;
    QSqlRecord record = query.record();
    int primaryKeyRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);

    while(query.next()) {
        int key = query.value(primaryKeyRecordIndex).toInt();

        QSharedPointer<QObject> currentObject;
        if (data->cache.contains(key)) {
            currentObject = data->cache.get(key);
        }

        if(!currentObject) {
            QObject *object = createInstance();
            data->storage->enableStorageFrom(object);
            currentObject = data->cache.insert(key, object);
            Qp::Private::enableSharedFromThis(currentObject);
            data->storage->sqlDataAccessObjectHelper()->readQueryIntoObject(query,
                                                                            record,
                                                                            currentObject.data());
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

Qp::UpdateResult QpDaoBase::updateObject(QSharedPointer<QObject> object)
{
    QObject *obj = object.data();
    int localRevision = data->storage->revisionInObject(obj);
    int remoteRevision = data->storage->revisionInDatabase(obj);

    if(localRevision < remoteRevision)
        return Qp::UpdateConflict;

    Q_ASSERT(localRevision == remoteRevision);

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
    if(mode == IgnoreRevision)
        return sync(object);

    QObject *obj = object.data();
    int localRevision = data->storage->revisionInObject(obj);
    int remoteRevision = data->storage->revisionInDatabase(obj);

    if(localRevision == remoteRevision)
        return Qp::Unchanged;

    Q_ASSERT(localRevision < remoteRevision);

    return sync(object);
}

bool QpDaoBase::synchronizeAllObjects()
{
    data->storage->database().transaction();

    foreach(QSharedPointer<QObject> object, readAllObjects(-1, -1, QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                                  QpSqlCondition::GreaterThan,
                                                                                  data->lastSynchronizedCreatedId))) {
        data->lastSynchronizedCreatedId = qMax(data->lastSynchronizedCreatedId, Qp::Private::primaryKey(object.data()));
        emit objectCreated(object);
    }


    foreach(QSharedPointer<QObject> object, readObjectsUpdatedAfterRevision(data->lastSynchronizedRevision)) {
        data->lastSynchronizedRevision = qMax(data->lastSynchronizedRevision, data->storage->revisionInObject(object.data()));

        QList<QpMetaProperty> rs = QpMetaObject::forObject(object).relationProperties();
        for(int i = 0, c = rs.size(); i < c; ++i) {
            QpMetaProperty relation = rs.at(i);
            QpRelationResolver::readRelationFromDatabase(relation, object.data());
        }

        if(Qp::Private::isDeleted(object.data()))
            emit objectMarkedAsDeleted(object);
        else
            emit objectUpdated(object);
    }

    data->storage->database().commit();

    if(lastError().isValid())
        return false;

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

int QpDaoBase::latestRevision() const
{
    return data->storage->sqlDataAccessObjectHelper()->latestRevision(data->metaObject);
}

#ifndef QP_NO_TIMESTAMPS
QList<QSharedPointer<QObject> > QpDaoBase::createdSince(const QDateTime &time)
{
    return createdSince(time.toString("yyyyMMddHHmmss.zzz").toDouble());
}

QList<QSharedPointer<QObject> > QpDaoBase::createdSince(double time)
{
    return readAllObjects(-1,-1, QString::fromLatin1("%1.%2 >= %3 ORDER BY %1.%2 ASC")
                          .arg(data->metaObject.tableName())
                          .arg(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME)
                          .arg(QString::number(time, 'f', 4)));
}

QList<QSharedPointer<QObject> > QpDaoBase::updatedSince(const QDateTime &time)
{
    return updatedSince(time.toString("yyyyMMddHHmmss.zzz").toDouble());
}

QList<QSharedPointer<QObject> > QpDaoBase::updatedSince(double time)
{
    return readAllObjects(-1,-1, QString::fromLatin1("%1.%2 >= %3 ORDER BY %1.%2 ASC")
                          .arg(data->metaObject.tableName())
                          .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
                          .arg(QString::number(time, 'f', 4)));
}
#endif
