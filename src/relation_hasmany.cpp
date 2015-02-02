#include "relation_hasmany.h"

#include "metaproperty.h"
#include "propertydependencies.h"
#include "qpersistence.h"
#include "relationresolver.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
#include <QDebug>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

/******************************************************************************
 * QpHasManyBase
 */
class QpHasManyData : public QSharedData {
public:
    QpHasManyData() :
        QSharedData(),
        resolved(false),
        owner(nullptr)
    {
    }

    bool resolved;
    QList<QSharedPointer<QObject>> objects;
    QpMetaProperty metaProperty;
    QObject *owner;
};


/******************************************************************************
 * QpHasManyBase
 */
QpHasManyBase::QpHasManyBase(const QString &name, QObject *parent) :
    data(new QpHasManyData)
{
    data->owner = parent;
    QString propertyName = QpMetaProperty::nameFromMaybeQualifiedName(name);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(propertyName);
}

QpHasManyBase::~QpHasManyBase()
{
}

bool QpHasManyBase::operator ==(const QList<QSharedPointer<QObject> > &objects) const
{
    return data->objects == objects;
}

QList<QSharedPointer<QObject> > QpHasManyBase::objects() const
{
    if (Qp::Private::primaryKey(data->owner) == 0)
        return QList<QSharedPointer<QObject> >();

    if (data->resolved)
        return data->objects;

    data->objects = QpRelationResolver::resolveToManyRelation(data->metaProperty.name(), data->owner);
    data->resolved = true;

    QpPropertyDependencies dependecies(QpStorage::forObject(data->owner));
    dependecies.initDependencies(data->owner, data->objects, data->metaProperty);

    return data->objects;
}

void QpHasManyBase::add(QSharedPointer<QObject> object)
{
    objects(); // resolve

    if (data->objects.contains(object))
        return;

    data->objects.append(object);

    if (object) {
        data->metaProperty.reverseRelation().add(object, Qp::sharedFrom(data->owner));

        QpPropertyDependencies dependecies(QpStorage::forObject(data->owner));
        dependecies.initDependencies(data->owner, object, data->metaProperty);
    }

    if (!data->objects.contains(object))
        data->objects.append(object);
}

void QpHasManyBase::remove(QSharedPointer<QObject> object)
{
    objects(); // resolve
    if(!data->objects.removeOne(object))
        return;

    if (object) {
        data->metaProperty.reverseRelation().remove(object, Qp::sharedFrom(data->owner));

        QpPropertyDependencies dependecies(QpStorage::forObject(data->owner));
        dependecies.removeDependencies(data->owner, object, data->metaProperty);
    }
}

void QpHasManyBase::setObjects(const QList<QSharedPointer<QObject>> objects) const
{
    data->objects = objects;
    data->resolved = true;

    QpPropertyDependencies dependecies(QpStorage::forObject(data->owner));
    dependecies.initDependencies(data->owner, objects, data->metaProperty);
}

bool QpHasManyBase::isResolved() const
{
    return data->resolved;
}
