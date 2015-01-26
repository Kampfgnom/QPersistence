#include "dataaccessobject.h"

#include "cache.h"
#include "databaseschema.h"
#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "propertydependencieshelper.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqldataaccessobjecthelper.h"
#include "sqlquery.h"
#include "storage.h"
#include "transactionshelper.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
#include <QSqlRecord>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


/******************************************************************************
 * QpDataAccessObjectBaseData
 */
class QpDataAccessObjectBaseData : public QSharedData
{
public:
    QpDataAccessObjectBaseData() :
        QSharedData(),
        lastSynchronizedCreatedId(0),
        lastSynchronizedRevision(0)
    {
    }

    QpStorage *storage;
    QpMetaObject metaObject;
    mutable QpCache cache;
    int lastSynchronizedCreatedId;
    int lastSynchronizedRevision;
};


/******************************************************************************
 * QpDataAccessObjectBase
 */
QpDataAccessObjectBase::QpDataAccessObjectBase(const QMetaObject &metaObject,
                     QpStorage *parent) :
    QObject(parent),
    data(new QpDataAccessObjectBaseData)
{
    data->storage = parent;
    data->metaObject = QpMetaObject::registerMetaObject(metaObject);
}

QpDataAccessObjectBase::~QpDataAccessObjectBase()
{
}

QpCache QpDataAccessObjectBase::cache() const
{
    return data->cache;
}

void QpDataAccessObjectBase::resetLastKnownSynchronization()
{
    if (!data->storage->beginTransaction())
        return;
    data->lastSynchronizedCreatedId = data->storage->sqlDataAccessObjectHelper()->maxPrimaryKey(data->metaObject);
    data->lastSynchronizedRevision = data->storage->sqlDataAccessObjectHelper()->latestRevision(data->metaObject);
    data->storage->commitOrRollbackTransaction();
}

Qp::SynchronizeResult QpDataAccessObjectBase::sync(QSharedPointer<QObject> object)
{
    QObject *obj = object.data();
    int id = data->storage->primaryKey(object);
    if (!data->storage->sqlDataAccessObjectHelper()->readObject(data->metaObject, id, obj)) {
        if (data->storage->lastError().isValid())
            return Qp::Error;

        data->cache.remove(data->storage->primaryKey(object));
        emit objectRemoved(object);
        return Qp::Removed;
    }

    foreach (QpMetaProperty relation, QpMetaObject::forObject(object).relationProperties()) {
        QpRelationResolver::readRelationFromDatabase(relation, obj);
    }

    emit objectSynchronized(object);

    if (Qp::Private::isDeleted(object.data())) {
        emit objectMarkedAsDeleted(object);
        return Qp::Deleted;
    }

    emit objectUpdated(object);
    return Qp::Updated;
}

QpMetaObject QpDataAccessObjectBase::qpMetaObject() const
{
    return data->metaObject;
}

int QpDataAccessObjectBase::count(const QpSqlCondition &condition) const
{
    return data->storage->sqlDataAccessObjectHelper()->count(data->metaObject, condition);
}

QList<int> QpDataAccessObjectBase::allKeys(int skip, int count) const
{
    QList<int> result = data->storage->sqlDataAccessObjectHelper()->allKeys(data->metaObject, skip, count);

    if (data->storage->lastError().isValid())
        return {};

    return result;
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::readAllObjects(int skip, int count, const QpSqlCondition &condition, QList<QpSqlQuery::OrderField> orders) const
{
    QpSqlQuery query = data->storage->sqlDataAccessObjectHelper()->readAllObjects(data->metaObject, skip, count, condition, orders);
    return readAllObjects(query);
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::readObjectsUpdatedAfterRevision(int revision) const
{
    QpSqlQuery query = data->storage->sqlDataAccessObjectHelper()->readObjectsUpdatedAfterRevision(data->metaObject, revision);
    return readAllObjects(query);
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::readAllObjects(QpSqlQuery &query) const
{
    if (data->storage->lastError().isValid())
        return {};

    QList<QSharedPointer<QObject> > result;
    QSqlRecord record = query.record();
    int primaryKeyRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    int revisionRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_REVISION);

    while (query.next()) {
        int key = query.value(primaryKeyRecordIndex).toInt();

        QSharedPointer<QObject> currentObject;
        if (data->cache.contains(key)) {
            currentObject = data->cache.get(key);
        }

        bool isNewObject = false;
        if (!currentObject) {
            isNewObject = true;
            currentObject = setupSharedObject(createInstance(), key);
            // Block signals, so that no signals are emitted for partly-initialized objects
            currentObject->blockSignals(true);
        }

        int localRevision = data->storage->revisionInObject(currentObject.data());
        int remoteRevision = query.value(revisionRecordIndex).toInt();

        // A revision of 0 means the object has no revision.
        // This should only be the case for objects, which have been created before enabling the history tracking.
        // We have to read these objects:
        if (remoteRevision <= 0
            || localRevision < remoteRevision) {
            data->storage->sqlDataAccessObjectHelper()->readQueryIntoObject(query,
                                                                            record,
                                                                            currentObject.data());
        }

        if (isNewObject) {
            currentObject->blockSignals(false);
            emit objectInstanceCreated(currentObject);
        }

        result.append(currentObject);
    }

    if (data->storage->lastError().isValid())
        return {};

    return result;
}

QSharedPointer<QObject> QpDataAccessObjectBase::readObject(int id) const
{
    if (id <= 0) {
        return QSharedPointer<QObject>();
    }

    QSharedPointer<QObject> p = data->cache.get(id);

    if (p)
        return p;

    QObject *object = createInstance();
    // Block signals, so that no signals are emitted for partly-initialized objects
    object->blockSignals(true);

    if (!data->storage->sqlDataAccessObjectHelper()->readObject(data->metaObject, id, object)) {
        delete object;
        return QSharedPointer<QObject>();
    }

    QSharedPointer<QObject> obj = setupSharedObject(object, id);
    object->blockSignals(false);
    emit objectInstanceCreated(obj);
    return obj;
}

QSharedPointer<QObject> QpDataAccessObjectBase::createObject()
{
    QObject *object = createInstance();
    // Block signals, so that no signals are emitted for partly-initialized objects
    object->blockSignals(true);

    if (!data->storage->sqlDataAccessObjectHelper()->insertObject(data->metaObject, object)) {
        delete object;
        return QSharedPointer<QObject>();
    }

    QSharedPointer<QObject> obj = setupSharedObject(object, Qp::Private::primaryKey(object));
    object->blockSignals(false);
    emit objectInstanceCreated(obj);
    emit objectCreated(obj);
    return obj;
}

Qp::UpdateResult QpDataAccessObjectBase::updateObject(QSharedPointer<QObject> object)
{
    QObject *obj = object.data();
    int localRevision = data->storage->revisionInObject(obj);
    int remoteRevision = data->storage->revisionInDatabase(obj);

    if (localRevision < remoteRevision)
        return Qp::UpdateConflict;

    Q_ASSERT(localRevision == remoteRevision);

    if (!data->storage->sqlDataAccessObjectHelper()->updateObject(data->metaObject, object.data()))
        return Qp::UpdateError;

    emit objectUpdated(object);
    return Qp::UpdateSuccess;
}

bool QpDataAccessObjectBase::removeObject(QSharedPointer<QObject> object)
{
    if (!data->storage->sqlDataAccessObjectHelper()->removeObject(data->metaObject, object.data()))
        return false;

    // We have to unlink all related objects, because otherwise the
    // object will still be referenced by all strong relations.
    // If I ever want to delete this line (again), I will have a look at the
    // libdBase2::Material::amount, which does not show the right value,
    // after removing storage contents, and reconsider deleting this line.
    unlinkRelations(object);

    data->cache.remove(data->storage->primaryKey(object));
    emit objectRemoved(object);
    return true;
}

bool QpDataAccessObjectBase::markAsDeleted(QSharedPointer<QObject> object)
{
    Qp::Private::markAsDeleted(object.data());
    Qp::UpdateResult result = updateObject(object);
    if (result != Qp::UpdateSuccess)
        return false;

    // See comment in removeObject for unlinkRelations
    unlinkRelations(object);

    emit objectMarkedAsDeleted(object);
    return true;
}

void QpDataAccessObjectBase::unlinkRelations(QSharedPointer<QObject> object) const
{
#ifdef __clang__
    _Pragma("clang diagnostic push");
    _Pragma("clang diagnostic ignored \"-Wshadow\"");
#endif
    foreach (QpMetaProperty relation, QpMetaObject::forObject(object).relationProperties()) {
        foreach (QSharedPointer<QObject> related, relation.read(object)) {
            relation.remove(object, related);
        }
    }
#ifdef __clang__
    _Pragma("clang diagnostic pop");
#endif
}

QSharedPointer<QObject> QpDataAccessObjectBase::setupSharedObject(QObject *object, int primaryKey) const
{
    data->storage->enableStorageFrom(object);
    QSharedPointer<QObject> shared = data->cache.insert(primaryKey, object);
    Qp::Private::enableSharedFromThis(shared);
    data->storage->propertyDependenciesHelper()->initSelfDependencies(shared);
    return shared;
}

bool QpDataAccessObjectBase::undelete(QSharedPointer<QObject> object)
{
    Qp::Private::undelete(object.data());
    blockSignals(true); // block object updated signal
    Qp::UpdateResult result = updateObject(object);
    blockSignals(false);
    if (result != Qp::UpdateSuccess)
        return false;

    emit objectUndeleted(object);
    return true;
}

Qp::SynchronizeResult QpDataAccessObjectBase::synchronizeObject(QSharedPointer<QObject> object, SynchronizeMode mode)
{
    if (mode == IgnoreRevision)
        return sync(object);

    QObject *obj = object.data();
    int localRevision = data->storage->revisionInObject(obj);
    int remoteRevision = data->storage->revisionInDatabase(obj);

    if (localRevision == remoteRevision)
        return Qp::Unchanged;

    Q_ASSERT(localRevision < remoteRevision);

    return sync(object);
}

bool QpDataAccessObjectBase::synchronizeAllObjects()
{
    if (!data->storage->beginTransaction())
        return false;

    foreach (QSharedPointer<QObject> object, readAllObjects(-1, -1, QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                                   QpSqlCondition::GreaterThan,
                                                                                   data->lastSynchronizedCreatedId))) {
        data->lastSynchronizedCreatedId = qMax(data->lastSynchronizedCreatedId, Qp::Private::primaryKey(object.data()));
        emit objectCreated(object);
    }

    foreach (QSharedPointer<QObject> object, readObjectsUpdatedAfterRevision(data->lastSynchronizedRevision)) {
        data->lastSynchronizedRevision = qMax(data->lastSynchronizedRevision, data->storage->revisionInObject(object.data()));

        QList<QpMetaProperty> rs = QpMetaObject::forObject(object).relationProperties();
        for (int i = 0, c = rs.size(); i < c; ++i) {
            QpMetaProperty relation = rs.at(i);
            QpRelationResolver::readRelationFromDatabase(relation, object.data());
        }

        emit objectSynchronized(object);

        if (Qp::Private::isDeleted(object.data()))
            emit objectMarkedAsDeleted(object);
        else
            emit objectUpdated(object);
    }

    if (!data->storage->commitOrRollbackTransaction()
        || data->storage->lastError().isValid())
        return false;

    return true;
}

bool QpDataAccessObjectBase::incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName)
{
    return data->storage->sqlDataAccessObjectHelper()->incrementNumericColumn(object.data(), fieldName);
}

int QpDataAccessObjectBase::latestRevision() const
{
    return data->storage->sqlDataAccessObjectHelper()->latestRevision(data->metaObject);
}

#ifndef QP_NO_TIMESTAMPS
QList<QSharedPointer<QObject> > QpDataAccessObjectBase::createdSince(const QDateTime &time)
{
    return createdSince(time.toString("yyyyMMddHHmmss.zzz").toDouble());
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::createdSince(double time)
{
    return readAllObjects(-1,-1, QString::fromLatin1("%1.%2 >= %3 ORDER BY %1.%2 ASC")
                          .arg(data->metaObject.tableName())
                          .arg(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME)
                          .arg(QString::number(time, 'f', 4)));
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::updatedSince(const QDateTime &time)
{
    return updatedSince(time.toString("yyyyMMddHHmmss.zzz").toDouble());
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::updatedSince(double time)
{
    return readAllObjects(-1,-1, QString::fromLatin1("%1.%2 >= %3 ORDER BY %1.%2 ASC")
                          .arg(data->metaObject.tableName())
                          .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
                          .arg(QString::number(time, 'f', 4)));
}
#endif
