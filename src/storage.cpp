#include "storage.h"

#include "sqlbackend.h"
#include "error.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

static const char *PROPERTY_STORAGE = "_Qp_storage";

class QpStorageData : public QSharedData
{
public:
    QpStorageData() :
        QSharedData(),
        locksEnabled(false)
    {}

    QpSqlDataAccessObjectHelper *sqlDataAccessObjectHelper;
    QSqlDatabase database;

    QpError lastError;
    bool locksEnabled;
    QHash<QSharedPointer<QObject>, QpLock> localLocks;
    QHash<QString, QVariant::Type> additionalLockFields;
    QHash<QString, QpDaoBase *> dataAccessObjects;
    QList<QpAbstractErrorHandler *> errorHandlers;

    static QpStorage *defaultStorage;
};

QpStorage *QpStorageData::defaultStorage(nullptr);
QpStorage *QpStorage::defaultStorage()
{
    if(!QpStorageData::defaultStorage)
        QpStorageData::defaultStorage = new QpStorage(Qp::Private::GlobalGuard());

    return QpStorageData::defaultStorage;
}

QpStorage::QpStorage(QObject *parent) :
    QObject(parent),
    data(new QpStorageData)
{
    data->sqlDataAccessObjectHelper = new QpSqlDataAccessObjectHelper(this);
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

void QpStorage::registerDataAccessObject(QpDaoBase *dao, const QMetaObject *objectInClassHierarchy)
{
    do {
        QString className = QpMetaObject::removeNamespaces(objectInClassHierarchy->className());
        data->dataAccessObjects.insert(objectInClassHierarchy->className(), dao);
        data->dataAccessObjects.insert(className, dao);

        objectInClassHierarchy = objectInClassHierarchy->superClass();
    } while(objectInClassHierarchy->className() != QObject::staticMetaObject.className());
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

QSqlDatabase QpStorage::database()
{
    return data->database;
}

bool QpStorage::adjustDatabaseSchema()
{
    beginTransaction();
    QpDatabaseSchema schema(this);
    schema.adjustSchema();
    return commitOrRollbackTransaction() == Qp::CommitSuccessful;
}

bool QpStorage::createCleanSchema()
{
    beginTransaction();
    QpDatabaseSchema schema(this);
    schema.createCleanSchema();
    return commitOrRollbackTransaction() == Qp::CommitSuccessful;
}

QpError QpStorage::lastError() const
{
    return data->lastError;
}

#ifdef __clang__
_Pragma("clang diagnostic push")
_Pragma("clang diagnostic ignored \"-Wmissing-noreturn\"")
#endif
void QpStorage::setLastError(QpError error)
{
    data->lastError = error;
    foreach(QpAbstractErrorHandler *handler, data->errorHandlers) {
        handler->handleError(error);
    }
}
#ifdef __clang__
_Pragma("clang diagnostic pop")
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

void QpStorage::setSqlDebugEnabled(bool enable)
{
    QpSqlQuery::setDebugEnabled(enable);
}

bool QpStorage::beginTransaction()
{
    if(data->database.driverName() != "QMYSQL")
        return true;

    bool transaction = data->database.transaction();
    if(!transaction)
        qFatal("START TRANSACTION failed.");

    if(QpSqlQuery::isDebugEnabled())
        qDebug() << "START TRANSACTION;";

    return transaction;
}

Qp::CommitResult QpStorage::commitOrRollbackTransaction()
{
    if(data->database.driverName() != "QMYSQL")
        return Qp::CommitSuccessful;

    if(lastError().isValid()) {
        bool rollback = data->database.rollback();
        if(!rollback)
            qFatal("ROLLBACK failed.");

        if(QpSqlQuery::isDebugEnabled())
            qDebug() << "ROLLBACK;";
        if(rollback)
            return Qp::RollbackSuccessful;
        else
            return Qp::RollbackFailed;
    }

    bool commit = data->database.commit();
    if(!commit) {
        qWarning() << data->database.lastError();
        qFatal("COMMIT failed.");
    }

    if(QpSqlQuery::isDebugEnabled())
        qDebug() << "COMMIT;";
    if(commit)
        return Qp::CommitSuccessful;
    else
        return Qp::CommitFailed;
}

QList<QpDaoBase *> QpStorage::dataAccessObjects()
{
    return data->dataAccessObjects.values();
}

QpDaoBase *QpStorage::dataAccessObject(const QMetaObject metaObject) const
{
    Q_ASSERT(data->dataAccessObjects.contains(metaObject.className()));
    return data->dataAccessObjects.value(metaObject.className());
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

    if (!query.exec()
            || query.lastError().isValid()) {
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

int QpStorage::revisionInDatabase(QObject *object)
{
    return sqlDataAccessObjectHelper()->objectRevision(object);
}

#ifndef QP_NO_TIMESTAMPS
double QpStorage::databaseTimeInternal()
{
    QpSqlQuery query(data->database);
    if(!query.exec(QString("SELECT %1").arg(QpSqlBackend::forDatabase(data->database)->nowTimestamp()))
            || !query.first()) {
        setLastError(QpError(query.lastError()));
        return -1.0;
    }

    return query.value(0).toDouble();
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
    return QDateTime::fromString(string, "yyyyMMddHHmmss.zzz");
}

QDateTime QpStorage::databaseTime()
{
    return dateFromDouble(databaseTimeInternal());
}
#endif
