#include <QPersistencePersistentDataAccessObject.h>

#include "error.h"
#include "metaobject.h"
#include "sqldataaccessobjecthelper.h"
#include "qpersistence.h"

#include <QtCore/QVariant>

class QPersistencePersistentDataAccessObjectBasePrivate : public QSharedData
{
public:
    QPersistenceSqlDataAccessObjectHelper *sqlDataAccessObjectHelper;
    QPersistenceMetaObject metaObject;
};

QPersistencePersistentDataAccessObjectBase::QPersistencePersistentDataAccessObjectBase(const QMetaObject &metaObject,
                                                               const QSqlDatabase &database,
                                                               QObject *parent) :
    QPersistenceAbstractDataAccessObject(parent),
    d(new QPersistencePersistentDataAccessObjectBasePrivate)
{
    d->sqlDataAccessObjectHelper = QPersistenceSqlDataAccessObjectHelper::forDatabase(database);
    d->metaObject = QPersistenceMetaObject(metaObject);
}

QPersistencePersistentDataAccessObjectBase::~QPersistencePersistentDataAccessObjectBase()
{
}

QPersistenceSqlDataAccessObjectHelper *QPersistencePersistentDataAccessObjectBase::sqlDataAccessObjectHelper() const
{
    return d->sqlDataAccessObjectHelper;
}

QPersistenceMetaObject QPersistencePersistentDataAccessObjectBase::dataSuiteMetaObject() const
{
    return d->metaObject;
}

int QPersistencePersistentDataAccessObjectBase::count() const
{
    return d->sqlDataAccessObjectHelper->count(d->metaObject);
}

QList<QVariant> QPersistencePersistentDataAccessObjectBase::allKeys() const
{
    QList<QVariant> result = d->sqlDataAccessObjectHelper->allKeys(d->metaObject);

    if(d->sqlDataAccessObjectHelper->lastError().isValid())
        setLastError(d->sqlDataAccessObjectHelper->lastError());

    return result;
}

QList<QObject *> QPersistencePersistentDataAccessObjectBase::readAllObjects() const
{
    QList<QObject *> result;
    Q_FOREACH(const QVariant key, allKeys()) result.append(readObject(key));
    return result;
}

QObject *QPersistencePersistentDataAccessObjectBase::readObject(const QVariant &key) const
{
    QObject *object = createObject();

    if(!d->sqlDataAccessObjectHelper->readObject(d->metaObject, key, object)) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        delete object;
        return nullptr;
    }

    return object;
}

bool QPersistencePersistentDataAccessObjectBase::insertObject(QObject * const object)
{
    if(!d->sqlDataAccessObjectHelper->insertObject(d->metaObject, object)) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        return false;
    }

    emit objectInserted(object);
    return true;
}

bool QPersistencePersistentDataAccessObjectBase::updateObject(QObject *const object)
{
    if(!d->sqlDataAccessObjectHelper->updateObject(d->metaObject, object)) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        return false;
    }

    emit objectUpdated(object);
    return true;
}

bool QPersistencePersistentDataAccessObjectBase::removeObject(QObject *const object)
{
    if(!d->sqlDataAccessObjectHelper->removeObject(d->metaObject, object)) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        return false;
    }

    emit objectRemoved(object);
    return true;
}


