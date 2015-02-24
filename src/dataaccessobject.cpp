#include "dataaccessobject.h"

#include "cache.h"
#include "databaseschema.h"
#include "datasource.h"
#include "datasourceresult.h"
#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "propertydependencieshelper.h"
#include "qpersistence.h"
#include "reply.h"
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

void QpDataAccessObjectBase::handleResultError()
{
    QpDatasourceResult *result = static_cast<QpDatasourceResult *>(sender());
    // The result sets storage's lastError, so that we have nothing to handle anything in here yet
    result->deleteLater();
}

QpDataAccessObjectBase::~QpDataAccessObjectBase()
{
}

QpCache QpDataAccessObjectBase::cache() const
{
    return data->cache;
}

QpStorage *QpDataAccessObjectBase::storage() const
{
    return data->storage;
}

void QpDataAccessObjectBase::resetLastKnownSynchronization()
{
    if (!data->storage->beginTransaction())
        return;

    QpDatasourceResult result(this);
    data->storage->datasource()->latestRevision(&result, data->metaObject);
    data->lastSynchronizedRevision = result.integerResult();
    result.reset();
    data->storage->datasource()->maxPrimaryKey(&result, data->metaObject);
    data->lastSynchronizedCreatedId = result.integerResult();

    data->storage->commitOrRollbackTransaction();
}

Qp::SynchronizeResult QpDataAccessObjectBase::sync(QSharedPointer<QObject> object, SynchronizeMode mode)
{
    QObject *obj = object.data();
    int primaryKey = Qp::Private::primaryKey(obj);
    QpDatasourceResult result(this);
    data->storage->datasource()->objectByPrimaryKey(&result, data->metaObject, primaryKey);

    if (result.lastError().isValid() || result.size() == 0) {
        data->cache.remove(data->storage->primaryKey(object));
        emit objectRemoved(object);

        if (data->storage->lastError().isValid())
            return Qp::Error;
        else
            return Qp::Removed;
    }
    Q_ASSERT(result.size() == 1);
    if(mode == RebaseMode) {
        QpDataTransferObjectDiff diff = result.dataTransferObjects().first().rebase(obj);
        if(diff.isConflict())
            return Qp::RebaseConflict;
    }
    else {
        result.dataTransferObjects().first().write(obj);
    }

    emit objectSynchronized(object);

    if (Qp::Private::isDeleted(object.data())) {
        emit objectMarkedAsDeleted(object);
        return Qp::Deleted;
    }

    emit objectUpdated(object);
    return Qp::Updated;
}

QpReply *QpDataAccessObjectBase::makeReply(QpDatasourceResult *result, std::function<void (QpDatasourceResult *result, QpReply *reply)> handleResult) const
{
    QpReply *reply = new QpReply(result, const_cast<QpDataAccessObjectBase *>(this));
    connect(result, &QpDatasourceResult::error, this, &QpDataAccessObjectBase::handleResultError);
    connect(result, &QpDatasourceResult::finished, [=] {
        handleResult(result, reply);
        result->deleteLater();
        reply->finish();
    });
    return reply;
}

QpMetaObject QpDataAccessObjectBase::qpMetaObject() const
{
    return data->metaObject;
}

int QpDataAccessObjectBase::count(const QpCondition &condition) const
{
    QpDatasourceResult result(this);
    data->storage->datasource()->count(&result, data->metaObject, condition);
    return result.integerResult();
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::readAllObjects(int skip, int limit, const QpCondition &condition, QList<QpDatasource::OrderField> orders) const
{
    QpDatasourceResult result(this);
    data->storage->datasource()->objects(&result, data->metaObject, skip, limit, condition, orders);
    return readObjects(&result);
}

QpReply *QpDataAccessObjectBase::readAllObjectsAsync(int skip, int limit, const QpCondition &condition, QList<QpDatasource::OrderField> orders) const
{
    QpDatasourceResult *result = new QpDatasourceResult(this);
    QMetaObject::invokeMethod(data->storage->asynchronousDatasource(), "objects",
                              Q_ARG(QpDatasourceResult *, result),
                              Q_ARG(QpMetaObject, data->metaObject),
                              Q_ARG(int, skip),
                              Q_ARG(int, limit),
                              Q_ARG(QpCondition, condition),
                              Q_ARG(QList<QpDatasource::OrderField>, orders));
    return makeReply(result, [this] (QpDatasourceResult *r, QpReply *reply) {
        reply->setObjects(readObjects(r));
    });
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::readAllObjects(const QList<int> primaryKeys) const
{
    if (primaryKeys.isEmpty())
        return {};

    QList<QSharedPointer<QObject> > result;
    QList<int> missing;
    foreach (int primaryKey, primaryKeys) {
        if (QSharedPointer<QObject> object = data->cache.get(primaryKey)) {
            result << object;
        }
        else {
            missing << primaryKey;
        }
    }

    if (!missing.isEmpty())
        result << readAllObjects(-1, -1, QpCondition::notDeletedAnd(QpCondition::primaryKeys(missing)));
    return result;
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::readObjectsUpdatedAfterRevision(int revision) const
{
    QpDatasourceResult result(this);
    data->storage->datasource()->objectsUpdatedAfterRevision(&result, data->metaObject, revision);
    return readObjects(&result);
}

QpReply *QpDataAccessObjectBase::readObjectsUpdatedAfterRevisionAsync(int revision) const
{
    QpDatasourceResult *result = new QpDatasourceResult(this);
    QMetaObject::invokeMethod(data->storage->asynchronousDatasource(), "objectsUpdatedAfterRevision",
                              Q_ARG(QpDatasourceResult *, result),
                              Q_ARG(QpMetaObject, data->metaObject),
                              Q_ARG(int, revision));

    return makeReply(result, [this] (QpDatasourceResult *r, QpReply *reply) {
        reply->setObjects(readObjects(r));
    });
}

QList<QSharedPointer<QObject> > QpDataAccessObjectBase::readObjects(QpDatasourceResult *datasourceResult) const
{
    if (datasourceResult->lastError().isValid())
        return {};

    QList<QSharedPointer<QObject> > result;
    foreach (const QpDataTransferObject &dto, datasourceResult->dataTransferObjects()) {
        int key = dto.primaryKey;

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

        dto.write(currentObject.data());

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

QSharedPointer<QObject> QpDataAccessObjectBase::readObject(int primaryKey) const
{
    if (primaryKey <= 0)
        return QSharedPointer<QObject>();

    QSharedPointer<QObject> p = data->cache.get(primaryKey);
    if (p)
        return p;

    QpDatasourceResult result(this);
    data->storage->datasource()->objectByPrimaryKey(&result, data->metaObject, primaryKey);

    if (result.lastError().isValid() || result.isEmpty())
        return QSharedPointer<QObject>();

    QObject *object = createInstance();
    // Block signals, so that no signals are emitted for partly-initialized objects
    object->blockSignals(true);

    Q_ASSERT(result.size() == 1);
    result.dataTransferObjects().first().write(object);

    QSharedPointer<QObject> obj = setupSharedObject(object, primaryKey);
    object->blockSignals(false);
    emit objectInstanceCreated(obj);
    return obj;
}

QSharedPointer<QObject> QpDataAccessObjectBase::createObject()
{
    QObject *object = createInstance();
    // Block signals, so that no signals are emitted for partly-initialized objects
    object->blockSignals(true);

    QpDatasourceResult result(this);
    data->storage->datasource()->insertObject(&result, object);

    if (result.lastError().isValid()) {
        delete object;
        return QSharedPointer<QObject>();
    }

    Q_ASSERT(result.size() == 1);
    result.dataTransferObjects().first().write(object);

    QSharedPointer<QObject> obj = setupSharedObject(object, Qp::Private::primaryKey(object));
    object->blockSignals(false);
    emit objectInstanceCreated(obj);
    emit objectCreated(obj);
    return obj;
}

Qp::UpdateResult QpDataAccessObjectBase::updateObject(QSharedPointer<QObject> object)
{
    QObject *obj = object.data();
    int localRevision = Qp::Private::revisionInObject(obj);
    int remoteRevision = revisionInDatabase(object);

    if (localRevision < remoteRevision) {
        Qp::SynchronizeResult syncResult = sync(object, RebaseMode);
        if(syncResult == Qp::Updated)
            return updateObject(object);
        else if(syncResult == Qp::RebaseConflict)
            return Qp::UpdateConflict;
        else
            return Qp::UpdateError;
    }

    Q_ASSERT(localRevision == remoteRevision);

    QpDatasourceResult result(this);
    data->storage->datasource()->updateObject(&result, obj);
    if (result.lastError().isValid())
        return Qp::UpdateError;

    if (result.isEmpty() && Qp::Private::isDeleted(object))
        return Qp::UpdateSuccess;

    emit objectUpdated(object);
    result.dataTransferObjects().first().write(obj);

    return Qp::UpdateSuccess;
}

bool QpDataAccessObjectBase::removeObject(QSharedPointer<QObject> object)
{
    // We have to unlink all related objects, because otherwise the
    // object will still be referenced by all strong relations.
    // If I ever want to delete this line (again), I will have a look at the
    // libdBase2::Material::amount, which does not show the right value,
    // after removing storage contents, and reconsider deleting this line.
    unlinkRelations(object);
    data->cache.remove(data->storage->primaryKey(object));

    QpDatasourceResult result(this);
    data->storage->datasource()->removeObject(&result, object.data());

    if (result.lastError().isValid())
        return false;

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
    int localRevision = Qp::Private::revisionInObject(obj);
    int remoteRevision = revisionInDatabase(object);

    if (localRevision == remoteRevision)
        return Qp::Unchanged;

    Q_ASSERT(localRevision < remoteRevision);

    return sync(object);
}

int QpDataAccessObjectBase::revisionInDatabase(QSharedPointer<QObject> object)
{
    QpDatasourceResult result(this);
    data->storage->datasource()->objectRevision(&result, object.data());
    if (result.lastError().isValid())
        return -1;
    return result.integerResult();
}

bool QpDataAccessObjectBase::synchronizeAllObjects()
{
    handleCreatedObjects(readAllObjects(-1, -1, QpCondition::notDeletedAnd(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                                       QpCondition::GreaterThan,
                                                                                       data->lastSynchronizedCreatedId))));
    handleUpdatedObjects(readObjectsUpdatedAfterRevision(data->lastSynchronizedRevision));

    return true;
}

QpReply *QpDataAccessObjectBase::synchronizeAllObjectsAsync()
{
    QpReply *reply = new QpReply(this);
    QpReply *r1 = readAllObjectsAsync(-1, -1, QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                          QpCondition::GreaterThan,
                                                          data->lastSynchronizedCreatedId));
    connect(r1, &QpReply::finished, [this, r1, reply] {
        handleCreatedObjects(r1->objects());
        r1->deleteLater();

        QpReply *r2 = readObjectsUpdatedAfterRevisionAsync(data->lastSynchronizedRevision);
        connect(r2, &QpReply::finished, [this, r2, reply] {
            handleUpdatedObjects(r2->objects());
            r2->deleteLater();

            reply->finish();
        });
    });

    return reply;
}

void QpDataAccessObjectBase::handleCreatedObjects(const QList<QSharedPointer<QObject> > &objects)
{
    foreach (QSharedPointer<QObject> object, objects) {
        data->lastSynchronizedCreatedId = qMax(data->lastSynchronizedCreatedId, Qp::Private::primaryKey(object.data()));
        emit objectCreated(object);
    }
}

void QpDataAccessObjectBase::handleUpdatedObjects(const QList<QSharedPointer<QObject> > &objects)
{
    foreach (QSharedPointer<QObject> object, objects) {
        QObject *obj = object.data();
        data->lastSynchronizedRevision = qMax(data->lastSynchronizedRevision, Qp::Private::revisionInObject(obj));

        emit objectSynchronized(object);

        if (Qp::Private::isDeleted(obj))
            emit objectMarkedAsDeleted(object);
        else
            emit objectUpdated(object);
    }
}

bool QpDataAccessObjectBase::incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName)
{
    QpDatasourceResult result(this);
    data->storage->datasource()->incrementNumericColumn(&result, object.data(), fieldName);
    if (result.lastError().isValid())
        return false;

    return true;
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
