#include "defaultstorage.h"

QP_DEFINE_STATIC_LOCAL(QpStorage, DefaultStorage)

QpStorage *Qp::defaultStorage() {
    static QpStorage *instance = nullptr;
    if(!instance)
        instance = new QpStorage;
    return instance;
}
void Qp::setDatabase(const QSqlDatabase &database) {
    Qp::defaultStorage()->setDatabase(database);
}
QSqlDatabase Qp::database() {
    return Qp::defaultStorage()->database();
}
bool Qp::adjustDatabaseSchema() {
    return Qp::defaultStorage()->adjustDatabaseSchema();
}
bool Qp::createCleanSchema() {
    return Qp::defaultStorage()->createCleanSchema();
}
QpError Qp::lastError() {
    return Qp::defaultStorage()->lastError();
}
void Qp::addErrorHandler(QpAbstractErrorHandler *handler) {
    Qp::defaultStorage()->addErrorHandler(handler);
}
void Qp::clearErrorHandlers() {
    Qp::defaultStorage()->clearErrorHandlers();
}

#ifndef QP_NO_LOCKS
bool Qp::unlockAllLocks() {
    return Qp::defaultStorage()->unlockAllLocks();
}
void Qp::enableLocks() {
    return Qp::defaultStorage()->enableLocks();
}
void Qp::addAdditionalLockInformationField(const QString &field, QVariant::Type type)
{
    return Qp::defaultStorage()->addAdditionalLockInformationField(field, type);
}
#endif

#ifndef QP_NO_TIMESTAMPS
QDateTime Qp::databaseTime() {
    return Qp::defaultStorage()->databaseTime();
}
#endif

void Qp::setSqlDebugEnabled(bool enable) {
    return Qp::defaultStorage()->setSqlDebugEnabled(enable);
}


