#ifndef QPERSISTENCE_H
#define QPERSISTENCE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "dataaccessobject.h"

template<class T>
class QpDao;
class QpDaoBase;
class QpError;
class QpLock;
class QpMetaObject;

namespace Qp {

enum CommitResult {
    RollbackSuccessful,
    RollbackFailed,
    CommitSuccessful,
    CommitFailed
};

enum SynchronizeResult : short {
    Error,
    Unchanged,
    Updated
};

enum UpdateResult : short {
    UpdateSuccess,
    UpdateConflict,
    UpdateError
};

void setDatabase(const QSqlDatabase &database);
QSqlDatabase database();
bool adjustDatabaseSchema();
bool createCleanSchema();
QpError lastError();
void enableLocks();
void addAdditionalLockInformationField(const QString &field, QVariant::Type type = QVariant::UserType);
QDateTime databaseTime();

bool beginTransaction();
CommitResult commitOrRollbackTransaction();

void setSqlDebugEnabled(bool enable);
void startBulkDatabaseQueries();
void commitBulkDatabaseQueries();

template<class T> int primaryKey(QSharedPointer<T> object);
template<class T> void registerClass();
template<class T> QpDao<T> *dataAccessObject();
template<class T> QSharedPointer<T> read(int id);
template<class T> QList<QSharedPointer<T> > readAll();
template<class T> int count();
template<class T> QSharedPointer<T> create();
template<class T> UpdateResult update(QSharedPointer<T> object);
template<class T> bool remove(QSharedPointer<T> object);
template<class T> SynchronizeResult synchronize(QSharedPointer<T> object);
template<class T> QList<QSharedPointer<T>> createdSince(const QDateTime &time);
template<class T> QList<QSharedPointer<T>> updatedSince(const QDateTime &time);
template<class T> QDateTime creationTimeInDatabase(QSharedPointer<T> object);
template<class T> QDateTime updateTimeInDatabase(QSharedPointer<T> object);
template<class T> QDateTime updateTimeInObject(QSharedPointer<T> object);
template<class T> QpLock tryLock(QSharedPointer<T> object, QHash<QString,QVariant> additionalInformation = QHash<QString,QVariant>());

template<class K, class V> void registerMappableTypes();
template<class T> void registerSetType();
template<class T> QSharedPointer<T> sharedFrom(const QObject *object);
template<typename T> QList<T> reversed( const QList<T> & in );
template<class Source, class Target>
QList<Target> castList(const QList<Source>& list);
template<class T, class O>
QList<QSharedPointer<T> > castList(const QList<QSharedPointer<O> >& list);

} // namespace Qp

#include "qpersistence_impl.cpp"

#define QPERSISTENCE_PROPERTYMETADATA "QPERSISTENCE_PROPERTYMETADATA"
#define QPERSISTENCE_SQLFILTER "QPERSISTENCE_SQLFILTER"
#define QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION "reverserelation"


#endif // QPERSISTENCE_H
