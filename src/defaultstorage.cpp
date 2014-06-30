#include "defaultstorage.h"

void Qp::setDatabase(const QSqlDatabase &database) { QpStorage::defaultStorage()->setDatabase(database); }
QSqlDatabase Qp::database() { return QpStorage::defaultStorage()->database(); }
bool Qp::adjustDatabaseSchema() { return QpStorage::defaultStorage()->adjustDatabaseSchema(); }
bool Qp::createCleanSchema() { return QpStorage::defaultStorage()->createCleanSchema(); }
QpError Qp::lastError() { return QpStorage::defaultStorage()->lastError(); }
void Qp::addErrorHandler(QpAbstractErrorHandler *handler) { QpStorage::defaultStorage()->addErrorHandler(handler); }
void Qp::clearErrorHandlers() { QpStorage::defaultStorage()->clearErrorHandlers(); }

#ifndef QP_NO_LOCKS
bool Qp::unlockAllLocks() { return QpStorage::defaultStorage()->unlockAllLocks(); }
void Qp::enableLocks() { return QpStorage::defaultStorage()->enableLocks(); }
void Qp::addAdditionalLockInformationField(const QString &field, QVariant::Type type)
{ return QpStorage::defaultStorage()->addAdditionalLockInformationField(field, type); }
#endif

#ifndef QP_NO_TIMESTAMPS
QDateTime Qp::databaseTime() { return QpStorage::defaultStorage()->databaseTime(); }
#endif

bool Qp::beginTransaction() { return QpStorage::defaultStorage()->beginTransaction(); }
Qp::CommitResult Qp::commitOrRollbackTransaction() { return QpStorage::defaultStorage()->commitOrRollbackTransaction(); }
void Qp::setSqlDebugEnabled(bool enable) { return QpStorage::defaultStorage()->setSqlDebugEnabled(enable); }


