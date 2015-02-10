#include "storage.h"

#include "datasource.h"
#include "datasourceresult.h"
#include "error.h"
#include "propertydependencieshelper.h"
#include "sqlbackend.h"
#include "sqlquery.h"
#include "transactionshelper.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
#include <QThread>
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
        locksEnabled(false),
        datasource(nullptr),
        asynchronousDatasource(nullptr),
        datasourceThread(nullptr)
    {
    }

    QSqlDatabase database;

    QpError lastError;
    bool locksEnabled;
    QHash<QSharedPointer<QObject>, QpLock> localLocks;
    QHash<QString, QVariant::Type> additionalLockFields;
    QHash<QString, QpDataAccessObjectBase *> dataAccessObjects;
    QList<QpAbstractErrorHandler *> errorHandlers;
    QpTransactionsHelper *transactionsHelper;
    QpPropertyDependenciesHelper *propertyDependenciesHelper;
    QpDatasource *datasource;
    QpDatasource *asynchronousDatasource;
    QThread *datasourceThread;

    static QpStorage *defaultStorage;
};


/******************************************************************************
 * QpStorage
 */
QpStorage::QpStorage(QObject *parent) :
    QObject(parent),
    data(new QpStorageData)
{
    data->transactionsHelper = new QpTransactionsHelper(this);
    data->propertyDependenciesHelper = new QpPropertyDependenciesHelper(this);

    qRegisterMetaType<QpDataTransferObjectsById>();
    qRegisterMetaType<QpMetaObject>();
    qRegisterMetaType<QpCondition>();
    qRegisterMetaType<QList<QpDatasource::OrderField>>();
    qRegisterMetaType<QSqlDatabase>();
    qRegisterMetaType<QpDatasourceResult *>();
    qRegisterMetaType<QpError>();
}

QpStorage::~QpStorage()
{
    delete data->transactionsHelper;
    delete data->propertyDependenciesHelper;
}

QpPropertyDependenciesHelper *QpStorage::propertyDependenciesHelper() const
{
    return data->propertyDependenciesHelper;
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

void QpStorage::registerDataAccessObject(QpDataAccessObjectBase *dao, const QMetaObject *objectInClassHierarchy)
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

    qDebug() << error;

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
    return data->transactionsHelper->begin();
}

bool QpStorage::commitOrRollbackTransaction()
{
    return data->transactionsHelper->commitOrRollback();
}

bool QpStorage::rollbackTransaction()
{
    return data->transactionsHelper->rollback();
}

void QpStorage::resetAllLastKnownSynchronizations()
{
    foreach (QpDataAccessObjectBase *dao, data->dataAccessObjects.values()) {
        dao->resetLastKnownSynchronization();
    }
}

QpDatasource *QpStorage::datasource() const
{
    Q_ASSERT_X(data->datasource, Q_FUNC_INFO, "you have to set a datasource");
    return data->datasource;
}

QpDatasource *QpStorage::asynchronousDatasource() const
{
    if(data->asynchronousDatasource)
        return data->asynchronousDatasource;

    Q_ASSERT(data->datasource->features() & QpDatasource::Asynchronous);
    data->datasourceThread = new QThread(const_cast<QpStorage *>(this));
    data->datasourceThread->setObjectName("DatasourceThread");
    data->asynchronousDatasource = datasource()->cloneForThread(data->datasourceThread);
    data->datasourceThread->start();
    return data->asynchronousDatasource;
}

void QpStorage::setDatasource(QpDatasource *datasource)
{
    if(data->datasource)
        data->datasource->deleteLater();

    if(data->asynchronousDatasource) {
        data->datasourceThread->quit();
        data->datasourceThread->deleteLater();
        data->asynchronousDatasource->deleteLater();
        data->datasourceThread = nullptr;
        data->asynchronousDatasource = nullptr;
    }

    data->datasource = datasource;
}

void QpStorage::setSqlDebugEnabled(bool enable)
{
    QpSqlQuery::setDebugEnabled(enable);
}

QList<QpDataAccessObjectBase *> QpStorage::dataAccessObjects()
{
    return data->dataAccessObjects.values();
}

QpDataAccessObjectBase *QpStorage::dataAccessObject(const QMetaObject metaObject) const
{
    return dataAccessObject(metaObject.className());
}

QpDataAccessObjectBase *QpStorage::dataAccessObject(const QString &className) const
{
    Q_ASSERT(data->dataAccessObjects.contains(className));
    return data->dataAccessObjects.value(className);
}

QpDataAccessObjectBase *QpStorage::dataAccessObject(int userType) const
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

QpDataAccessObjectBase *QpStorage::dataAccessObject(QSharedPointer<QObject> object) const
{
    return dataAccessObject(*object->metaObject());
}

bool QpStorage::incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName)
{
    QpDataAccessObjectBase *dao = dataAccessObject(object);
    if (!dao->incrementNumericColumn(object, fieldName))
        return false;

    return dao->synchronizeObject(object, QpDataAccessObjectBase::IgnoreRevision) == Qp::Updated;
}

Qp::UpdateResult QpStorage::update(QSharedPointer<QObject> object)
{
    return dataAccessObject(object)->updateObject(object);
}

Qp::SynchronizeResult QpStorage::synchronize(QSharedPointer<QObject> object, QpDataAccessObjectBase::SynchronizeMode mode)
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

double QpStorage::creationTimeInInObject(QObject *object)
{
    return object->property(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME).toDouble();
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
