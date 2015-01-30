#include "relations.h"

#include "datasourceresult.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "propertydependencieshelper.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

/******************************************************************************
 * QpRelationBase
 */
class QpRelationBasePrivate : public QSharedData
{
public:
    QpRelationBasePrivate() : QSharedData(), strong(false) {
    }
    QpMetaProperty metaProperty;
    QObject *owner;
    bool strong;

    void initDependencies(QSharedPointer<QObject> object) const;
    void removeDependencies(QSharedPointer<QObject> object) const;
};

void QpRelationBasePrivate::initDependencies(QSharedPointer<QObject> object) const
{
    QpStorage::forObject(owner)->propertyDependenciesHelper()->initDependencies(owner, object, metaProperty);
}

void QpRelationBasePrivate::removeDependencies(QSharedPointer<QObject> object) const
{
    QpStorage::forObject(owner)->propertyDependenciesHelper()->removeDependencies(owner, object, metaProperty);
}

QpRelationBase::QpRelationBase(QpRelationBasePrivate &dd, const QString &name, QObject *owner) :
    d_ptr(&dd)
{
    Q_D(QpRelationBase);
    d->owner = owner;
    QString propertyName = QpMetaProperty::nameFromMaybeQualifiedName(name);
    d->metaProperty = QpMetaObject::forObject(owner).metaProperty(propertyName);
}

QpRelationBase::~QpRelationBase()
{
}

void QpRelationBase::setStrong(bool strong)
{
    Q_D(QpRelationBase);
    d->strong = strong;
}


/******************************************************************************
 * QpRelationToOneBase
 */
class QpRelationToManyBasePrivate : public QpRelationBasePrivate
{
public:
    QpRelationToManyBasePrivate() : QpRelationBasePrivate(), resolved(false) {
    }

    QList<int> foreignKeys() const;

    QList<QSharedPointer<QObject> > resolve() const;
    QList<int> foreignKeyFromDataTransferObject() const;

    QList<QSharedPointer<QObject> > objects(bool *ok) const;
    void setObjects(QList<QSharedPointer<QObject> > objects) const;
    void addObject(QSharedPointer<QObject> object) const;
    bool removeObject(QSharedPointer<QObject> object) const;

    void initMultipleDependencies(QList<QSharedPointer<QObject> > objects) const;

    mutable bool resolved;

private:
    mutable QList<QWeakPointer<QObject> > weakObjects;
    mutable QList<QSharedPointer<QObject> > sharedObjects;

    mutable QList<int> fks;
};

void QpRelationToManyBasePrivate::initMultipleDependencies(QList<QSharedPointer<QObject> > objects) const
{
    QpStorage::forObject(owner)->propertyDependenciesHelper()->initDependencies(owner, objects, metaProperty);
}

QList<int> QpRelationToManyBasePrivate::foreignKeys() const
{
    if (resolved)
        return fks;

    QpDataTransferObject dto = QpDataTransferObject::fromObject(owner);
    return dto.toManyRelationFKs.value(metaProperty.metaProperty().propertyIndex());
}

QList<QSharedPointer<QObject> > QpRelationToManyBasePrivate::resolve() const
{
    QList<int> localFks = foreignKeys();
    if (localFks.isEmpty())
        return {};

    QpStorage *storage = QpStorage::forObject(owner);
    QpMetaObject foreignMetaObject = metaProperty.reverseMetaObject();
    QpDataAccessObjectBase *dao = storage->dataAccessObject(foreignMetaObject.metaObject());
    return dao->readAllObjects(localFks);
}

QList<QSharedPointer<QObject> > QpRelationToManyBasePrivate::objects(bool *ok) const
{
    if (strong) {
        *ok = true;
        return sharedObjects;
    }
    else {
        return Qp::Private::makeListStrong(weakObjects, ok);
    }
}

void QpRelationToManyBasePrivate::setObjects(QList<QSharedPointer<QObject> > objects) const
{
    fks = Qp::Private::primaryKeys(objects);
    resolved = true;
    if (strong)
        sharedObjects = objects;
    else
        weakObjects = Qp::Private::makeListWeak(objects);
}

void QpRelationToManyBasePrivate::addObject(QSharedPointer<QObject> object) const
{
    Q_ASSERT(resolved);
    int fk = Qp::Private::primaryKey(object);
    fks << fk;
    if (strong)
        sharedObjects << object;
    else
        weakObjects << object.toWeakRef();
}

bool QpRelationToManyBasePrivate::removeObject(QSharedPointer<QObject> object) const
{
    Q_ASSERT(resolved);
    int fk = Qp::Private::primaryKey(object);
    fks.removeOne(fk);
    if (strong)
        return sharedObjects.removeOne(object);
    else
        return weakObjects.removeOne(object.toWeakRef());
}

QpRelationToManyBase::QpRelationToManyBase(const QString &name, QObject *owner) :
    QpRelationBase(*new QpRelationToManyBasePrivate, name, owner)
{
}

QpRelationToManyBase::~QpRelationToManyBase()
{
}

QList<QSharedPointer<QObject> > QpRelationToManyBase::objects() const
{
    Q_D(const QpRelationToManyBase);

    if (d->resolved) {
        bool ok = false;
        QList<QSharedPointer<QObject> > os = d->objects(&ok);
        if (ok)
            return os;
    }

    QList<QSharedPointer<QObject> > objects = d->resolve();
    d->setObjects(objects);
    d->initMultipleDependencies(objects);

    return objects;
}

void QpRelationToManyBase::add(QSharedPointer<QObject> object)
{
    Q_D(QpRelationToManyBase);

    QList<QSharedPointer<QObject> > objs = objects(); // resolve

    if (objs.contains(object))
        return;

    d->addObject(object);
    d->metaProperty.reverseRelation().add(object, Qp::sharedFrom(d->owner));
    d->initDependencies(object);
}

void QpRelationToManyBase::remove(QSharedPointer<QObject> object)
{
    Q_D(QpRelationToManyBase);


    QList<QSharedPointer<QObject> > objs = objects(); Q_UNUSED(objs); // resolve

    if (!d->removeObject(object))
        return;

    d->metaProperty.reverseRelation().remove(object, Qp::sharedFrom(d->owner));
    d->removeDependencies(object);
}

bool QpRelationToManyBase::isResolved() const
{
    Q_D(const QpRelationToManyBase);
    return d->resolved;
}

void QpRelationToManyBase::setObjects(const QList<QSharedPointer<QObject> > &objects)
{
    Q_D(QpRelationToManyBase);

    d->setObjects(objects);
    d->initMultipleDependencies(objects);
}


/******************************************************************************
 * QpRelationToOneBase
 */
class QpRelationToOneBasePrivate : public QpRelationBasePrivate
{
public:
    QpRelationToOneBasePrivate() : QpRelationBasePrivate(), fk(-1) {
    }

    int foreignKey() const;

    QSharedPointer<QObject> resolve() const;

    QSharedPointer<QObject> object() const;
    void setObject(QSharedPointer<QObject> object) const;

    bool isLocallyChanged() const;

private:
    mutable QWeakPointer<QObject> weakObject;
    mutable QSharedPointer<QObject> sharedObject;

    mutable int fk;
};

int QpRelationToOneBasePrivate::foreignKey() const
{
    if (isLocallyChanged())
        return fk;

    QpDataTransferObject dto = QpDataTransferObject::fromObject(owner);
    return dto.toOneRelationFKs.value(metaProperty.metaProperty().propertyIndex());
}

QSharedPointer<QObject> QpRelationToOneBasePrivate::resolve() const
{
    int localFk = foreignKey();
    if (localFk <= 0)
        return QSharedPointer<QObject>();

    QpStorage *storage = QpStorage::forObject(owner);
    QpMetaObject foreignMetaObject = metaProperty.reverseMetaObject();
    QpDataAccessObjectBase *dao = storage->dataAccessObject(foreignMetaObject.metaObject());
    return dao->readObject(localFk);
}

QSharedPointer<QObject> QpRelationToOneBasePrivate::object() const
{
    if (strong)
        return sharedObject;
    else
        return weakObject.toStrongRef();
}

void QpRelationToOneBasePrivate::setObject(QSharedPointer<QObject> object) const
{
    fk = object ? Qp::Private::primaryKey(object.data()) : 0;
    if (strong)
        sharedObject = object;
    else
        weakObject = object.toWeakRef();
}

bool QpRelationToOneBasePrivate::isLocallyChanged() const
{
    return fk >= 0;
}

QpRelationToOneBase::QpRelationToOneBase(const QString &name, QObject *owner) :
    QpRelationBase(*new QpRelationToOneBasePrivate, name, owner)
{
}

QpRelationToOneBase::~QpRelationToOneBase()
{
}

bool QpRelationToOneBase::operator ==(const QSharedPointer<QObject> &object) const
{
    Q_D(const QpRelationToOneBase);
    return d->object() == object;
}

QSharedPointer<QObject> QpRelationToOneBase::object() const
{
    Q_D(const QpRelationToOneBase);

    QSharedPointer<QObject> object = d->object();
    if (object)
        return object;

    object = d->resolve();
    d->setObject(object);
    if (object)
        d->initDependencies(object);

    return object;
}

void QpRelationToOneBase::setObject(const QSharedPointer<QObject> newObject)
{
    Q_D(QpRelationToOneBase);

    int newFk = newObject ? Qp::Private::primaryKey(newObject) : 0;
    if (d->foreignKey() == newFk)
        return;

    QSharedPointer<QObject> previousObject = object();
    if (previousObject == newObject)
        return;

    d->setObject(newObject);

    QpMetaProperty reverse = d->metaProperty.reverseRelation();
    QSharedPointer<QObject> sharedOwner = Qp::sharedFrom(d->owner);

    if (previousObject) {
        reverse.remove(previousObject, sharedOwner);
        d->removeDependencies(previousObject);
    }

    if (newObject) {
        reverse.add(newObject, sharedOwner);
        d->initDependencies(newObject);
    }

    // Set again, because it may happen, that resetting the previousObjects relation has also reset this value.
    d->setObject(newObject);
}
