#include "relation_belongstomany.h"

#include "metaproperty.h"
#include "propertydependencieshelper.h"
#include "qpersistence.h"
#include "relationresolver.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
#include <QDebug>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

/******************************************************************************
 * QpBelongsToManyData
 */
class QpBelongsToManyData : public QSharedData {
public:
    QpBelongsToManyData() :
        QSharedData(),
        resolved(false),
        owner(nullptr)
    {
    }

    bool resolved;
    QList<QWeakPointer<QObject> > objects;
    QpMetaProperty metaProperty;
    QObject *owner;
};


/******************************************************************************
 * QpBelongsToManyBase
 */
QpBelongsToManyBase::QpBelongsToManyBase(const QString &name, QObject *parent) :
    data(new QpBelongsToManyData)
{
    data->owner = parent;
    QString propertyName = QpMetaProperty::nameFromMaybeQualifiedName(name);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(propertyName);
}

QpBelongsToManyBase::~QpBelongsToManyBase()
{
}

bool QpBelongsToManyBase::operator ==(const QList<QSharedPointer<QObject> > &objects) const
{
    return Qp::Private::makeListStrong(data->objects) == objects;
}

QList<QSharedPointer<QObject> > QpBelongsToManyBase::objects() const
{
    if (Qp::Private::primaryKey(data->owner) == 0)
        return QList<QSharedPointer<QObject> >();

    if (data->resolved) {
        bool ok = false;
        QList<QSharedPointer<QObject> > objs = Qp::Private::makeListStrong(data->objects, &ok);
        if (ok)
            return objs;
    }

    QList<QSharedPointer<QObject> > objects = QpRelationResolver::resolveToManyRelation(data->metaProperty.name(), data->owner);
    data->objects = Qp::Private::makeListWeak(objects);
    data->resolved = true;

    QpStorage::forObject(data->owner)->propertyDependenciesHelper()->initDependencies(data->owner, objects, data->metaProperty);

    return objects;
}

void QpBelongsToManyBase::add(QSharedPointer<QObject> object)
{
    QList<QSharedPointer<QObject> > obj = objects(); Q_UNUSED(obj); // resolve and keep a strong ref, while we're working here

    QWeakPointer<QObject> weakRef = object.toWeakRef();
    if (data->objects.contains(weakRef))
        return;

    data->objects.append(weakRef);
    if (object) {
        data->metaProperty.reverseRelation().add(object, Qp::sharedFrom(data->owner));

        QpStorage::forObject(data->owner)->propertyDependenciesHelper()->initDependencies(data->owner, object, data->metaProperty);
    }
}

void QpBelongsToManyBase::remove(QSharedPointer<QObject> object)
{
    QList<QSharedPointer<QObject> > obj = objects(); Q_UNUSED(obj); // resolve and keep a strong ref, while we're working here

    if (!data->objects.removeOne(object.toWeakRef()))
        return;

    if (object) {
        data->metaProperty.reverseRelation().remove(object, Qp::sharedFrom(data->owner));

        QpStorage::forObject(data->owner)->propertyDependenciesHelper()->removeDependencies(data->owner, object, data->metaProperty);
    }
}

void QpBelongsToManyBase::setObjects(const QList<QSharedPointer<QObject> > &objects) const
{
    data->objects = Qp::Private::makeListWeak(objects);
    data->resolved = true;

    QpStorage::forObject(data->owner)->propertyDependenciesHelper()->initDependencies(data->owner, objects, data->metaProperty);
}
