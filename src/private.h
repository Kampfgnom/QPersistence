#ifndef PRIVATE_H
#define PRIVATE_H

#include <QPersistencePersistentDataAccessObject.h>

namespace Qp {

namespace Private {

const QString QPERSISTENCE_SHARED_POINTER_PROPERTY("_Qp_sharedpointer");

void registerDataAccessObject(QpDaoBase *dataAccessObject,
                              const QMetaObject &metaObject);

QpMetaObject metaObject(const QString &className);
QList<QpMetaObject> metaObjects();

QpDaoBase *dataAccessObject(const QMetaObject &metaObject);
QList<QpDaoBase *> dataAccessObjects();

void enableSharedFromThis(QSharedPointer<QObject> object);

int primaryKey(QObject *object);
void setPrimaryKey(QObject *object, int key);

} // namespace Private

} // namespace Qp

#endif // PRIVATE_H
    
