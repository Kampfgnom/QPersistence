#include "storage.h"

#include "sqlbackend.h"
#include "error.h"
#include "sqlquery.h"
#include "transactionshelper.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

static const char *PROPERTY_STORAGE = "_Qp_storage";


/******************************************************************************
 * QpStorageData
 */
class QpStorageData : public QSharedData
{
public:
    QpStorageData() :
        QSharedData(),
        locksEnabled(false)
    {
    }

    QpSqlDataAccessObjectHelper *sqlDataAccessObjectHelper;
    QSqlDatabase database;

    QpError lastError;
    bool locksEnabled;
    QHash<QSharedPointer<QObject>, QpLock> localLocks;
    QHash<QString, QVariant::Type> additionalLockFields;
    QHash<QString, QpDaoBase *> dataAccessObjects;
    QList<QpAbstractErrorHandler *> errorHandlers;
    QpTransactionsHelper transactions;

    static QpStorage *defaultStorage;
};

QpStorage *QpStorageData::defaultStorage(nullptr);


/******************************************************************************
 * QpStorage
 */
QpStorage *QpStorage::defaultStorage()
{
    if (!QpStorageData::defaultStorage)
        QpStorageData::defaultStorage = new QpStorage(Qp::Private::GlobalGuard());

    return QpStorageData::defaultStorage;
}

QpStorage::QpStorage(QObject *parent) :
    QObject(parent),
    data(new QpStorageData)
{
    data->sqlDataAccessObjectHelper = new QpSqlDataAccessObjectHelper(this);
    data->transactions = QpTransactionsHelper::forStorage(this);
}

QpStorage::~QpStorage()
{
}

QpSqlDataAccessObjectHelper *QpStorage::sqlDataAccessObjectHelper() const
{
    return data->sqlDataAccessObjectHelper;
}

void QpStorage::enableStorageFrom(QObject *object)
{
    object->setProperty(PROPERTY_STORAGE, QVariant::fromValue<QpStorage *>(this));
}

QpStorage *QpStorage::forObject(const QObject *object)
{
    return object->property(PROPERTY_STORAGE).value<QpStorage *>();
}

QpStorage *QpStorage::forObject(QSharedPointer<QObject> object)
{
    return object->property(PROPERTY_STORAGE).value<QpStorage *>();
}

int QpStorage::revisionInDatabase(QObject *object)
{
    return sqlDataAccessObjectHelper()->objectRevision(object);
}

void QpStorage::registerDataAccessObject(QpDaoBase *dao, const QMetaObject *objectInClassHierarchy)
{
    do {
        QString className = QpMetaObject::removeNamespaces(objectInClassHierarchy->className());
        data->dataAccessObjects.insert(objectInClassHierarchy->className(), dao);
        data->dataAccessObjects.insert(className, dao);

        objectInClassHierarchy = objectInClassHierarchy->superClass();
    } while (objectInClassHierarchy->className() != QObject::staticMetaObject.className());
}

void QpStorage::setDatabase(const QSqlDatabase &database)
{
    if (data->database.isOpen()) {
        data->database.close();
    }

    data->database = database;

#ifdef QP_FOR_SQLITE
    QpSqlQuery query(database);
    query.prepare("PRAGMA foreign_keys = 1;");
    if (!query.exec()
        || query.lastError().isValid()) {
        qCritical() << "The PRAGMA foreign_keys could not be set to 1:" << query.lastError();
    }
#endif
}

QSqlDatabase QpStorage::database() const
{
    return data->database;
}

bool QpStorage::adjustDatabaseSchema()
{
    QpDatabaseSchema schema(this);
    return schema.adjustSchema();
}

bool QpStorage::createCleanSchema()
{
    QpDatabaseSchema schema(this);
    return schema.createCleanSchema();
}

QpError QpStorage::lastError() const
{
    return data->lastError;
}

void QpStorage::setLastError(const QSqlQuery &query)
{
    setLastError(QpError(query.lastError()));
}

#ifdef __clang__
_Pragma("clang diagnostic push");
_Pragma("clang diagnostic ignored \"-Wmissing-noreturn\"");
#endif
void QpStorage::setLastError(const QpError &error)
{
    data->lastError = error;
    if (!error.isValid())
        return;

    foreach (QpAbstractErrorHandler *handler, data->errorHandlers) {
        handler->handleError(error);
    }
}
#ifdef __clang__
_Pragma("clang diagnostic pop");
#endif

void QpStorage::addErrorHandler(QpAbstractErrorHandler *handler)
{
    data->errorHandlers.append(handler);
    handler->setParent(this);
}

void QpStorage::clearErrorHandlers()
{
    qDeleteAll(data->errorHandlers);
    data->errorHandlers.clear();
}

bool QpStorage::beginTransaction()
{
    return data->transactions.begin();
}

bool QpStorage::commitOrRollbackTransaction()
{
    return data->transactions.commitOrRollback();
}

bool QpStorage::rollbackTransaction()
{
    return data->transactions.rollback();
}

void QpStorage::resetAllLastKnownSynchronizations()
{
    foreach (QpDaoBase *dao, data->dataAccessObjects.values()) {
        dao->resetLastKnownSynchronization();
    }
}

void QpStorage::setSqlDebugEnabled(bool enable)
{
    QpSqlQuery::setDebugEnabled(enable);
}

QList<QpDaoBase *> QpStorage::dataAccessObjects()
{
    return data->dataAccessObjects.values();
}

QpDaoBase *QpStorage::dataAccessObject(const QMetaObject metaObject) const
{
    return dataAccessObject(metaObject.className());
}

QpDaoBase *QpStorage::dataAccessObject(const QString &className) const
{
    Q_ASSERT(data->dataAccessObjects.contains(className));
    return data->dataAccessObjects.value(className);
}

QpDaoBase *QpStorage::dataAccessObject(int userType) const
{
    return dataAccessObject(Qp::Private::classNameForUserType(userType));
}

#ifndef QP_NO_LOCKS
bool QpStorage::isLocksEnabled()
{
    return data->locksEnabled;
}

bool QpStorage::unlockAllLocks()
{
    QpSqlQuery query(data->database);
    query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
    query.prepareDelete();

    if (!query.exec()) {
        setLastError(query.lastError());
        return false;
    }

    return true;
}

void QpStorage::addAdditionalLockInformationField(const QString &name, QVariant::Type type)
{
    data->additionalLockFields.insert(name, type);
}

QHash<QString, QVariant::Type> QpStorage::additionalLockInformationFields()
{
    return data->additionalLockFields;
}

void QpStorage::enableLocks()
{
    data->locksEnabled = true;
}
#endif

int QpStorage::revisionInObject(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_REVISION).toInt();
}

QpDaoBase *QpStorage::dataAccessObject(QSharedPointer<QObject> object) const
{
    return dataAccessObject(*object->metaObject());
}

bool QpStorage::incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName)
{
    QpDaoBase *dao = dataAccessObject(object);
    if (!dao->incrementNumericColumn(object, fieldName))
        return false;

    return dao->synchronizeObject(object, QpDaoBase::IgnoreRevision) == Qp::Updated;
}

Qp::UpdateResult QpStorage::update(QSharedPointer<QObject> object)
{
    Qp::UpdateResult result = dataAccessObject(object)->updateObject(object);
    Q_ASSERT(result == Qp::UpdateSuccess);
    return result;
}

Qp::SynchronizeResult QpStorage::synchronize(QSharedPointer<QObject> object, QpDaoBase::SynchronizeMode mode)
{
    return dataAccessObject(object)->synchronizeObject(object, mode);
}

bool QpStorage::remove(QSharedPointer<QObject> object)
{
    return dataAccessObject(object)->removeObject(object);
}

int QpStorage::primaryKey(QSharedPointer<QObject> object)
{
    return Qp::Private::primaryKey(object.data());
}

bool QpStorage::isDeleted(QSharedPointer<QObject> object)
{
    return Qp::Private::isDeleted(object.data());
}

bool QpStorage::markAsDeleted(QSharedPointer<QObject> object)
{
    return dataAccessObject(object)->markAsDeleted(object);
}

bool QpStorage::undelete(QSharedPointer<QObject> object)
{
    return dataAccessObject(object)->undelete(object);
}

#ifndef QP_NO_TIMESTAMPS
double QpStorage::databaseTimeInternal()
{
    QpSqlQuery query(data->database);
    if (!query.exec(QString("SELECT %1").arg(QpSqlBackend::forDatabase(data->database)->nowTimestamp()))
        || !query.first()) {
        setLastError(QpError(query.lastError()));
        return -1.0;
    }

    return query.value(0).toDouble();
}

double QpStorage::creationTime(QObject *object)
{
    double time = creationTimeInInObject(object);
#ifdef __clang__
    _Pragma("GCC diagnostic push");
    _Pragma("GCC diagnostic ignored \"-Wused-but-marked-unused\"");
#endif
    if (qFuzzyCompare(0.0, time))
        return creationTimeInDatabase(object);
#ifdef __clang__
    _Pragma("GCC diagnostic pop");
#endif
    return time;
}

double QpStorage::creationTimeInDatabase(QObject *object)
{
    return sqlDataAccessObjectHelper()->readCreationTime(object);
}

double QpStorage::creationTimeInInObject(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME).toDouble();
}

double QpStorage::updateTimeInDatabase(QObject *object)
{
    return sqlDataAccessObjectHelper()->readUpdateTime(object);
}

double QpStorage::updateTimeInObject(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME).toDouble();
}

QDateTime dateFromDouble(double value)
{
    QString string = QString("%1").arg(value, 17, 'f', 3);
    QDateTime time = QDateTime::fromString(string, "yyyyMMddHHmmss.zzz");

    // TODO: Query the DB's timezone
    time.setTimeSpec(Qt::UTC);
    return time.toLocalTime();
}

QDateTime QpStorage::databaseTime()
{
    return dateFromDouble(databaseTimeInternal());
}
#endif
