#include "relation_hasone.h"

#include "metaproperty.h"
#include "propertydependencieshelper.h"
#include "qpersistence.h"
#include "relationresolver.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


/******************************************************************************
 * QpHasOneData
 */
class QpHasOneData : public QSharedData {
public:
    QpHasOneData() :
        QSharedData(),
        owner(nullptr),
        resolved(false)
    {
    }

    QSharedPointer<QObject> object;
    QpMetaProperty metaProperty;
    QObject *owner;
    bool resolved;
};


/******************************************************************************
 * QpHasOneBase
 */
QpHasOneBase::QpHasOneBase(const QString &name, QObject *parent) :
    data(new QpHasOneData)
{
    data->owner = parent;
    QString propertyName = QpMetaProperty::nameFromMaybeQualifiedName(name);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(propertyName);
}

QpHasOneBase::~QpHasOneBase()
{
}

bool QpHasOneBase::operator ==(const QSharedPointer<QObject> &object) const
{
    return data->object == object;
}

QSharedPointer<QObject> QpHasOneBase::objectWithoutResolving() const
{
    return data->object;
}

QSharedPointer<QObject> QpHasOneBase::object() const
{
    if (Qp::Private::primaryKey(data->owner) == 0)
        return QSharedPointer<QObject>();

    if (data->resolved)
        return data->object;

    data->object = QpRelationResolver::resolveToOneRelation(data->metaProperty.name(), data->owner);
    data->resolved = true;

    QpStorage::forObject(data->owner)->propertyDependenciesHelper()->initDependencies(data->owner, data->object, data->metaProperty);

    return data->object;
}

void QpHasOneBase::setObject(const QSharedPointer<QObject> newObject) const
{
    QSharedPointer<QObject> previousObject = object();

    if (previousObject == newObject)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    data->object = newObject;
    QpPropertyDependenciesHelper *dependeciesHelper = QpStorage::forObject(data->owner)->propertyDependenciesHelper();

    QSharedPointer<QObject> sharedOwner = Qp::sharedFrom(data->owner);
    if (previousObject) {
        reverse.remove(previousObject, sharedOwner);
        dependeciesHelper->removeDependencies(data->owner, previousObject, data->metaProperty);
    }

    if (newObject) {
        reverse.add(newObject, sharedOwner);
        dependeciesHelper->initDependencies(data->owner, newObject, data->metaProperty);
    }

    // Set again, because it may (will) happen, that setting the reverse relations has also changed this value.
    data->object = newObject;

    // Set dynamic property for relation
    QByteArray column;
    if (data->metaProperty.hasTableForeignKey())
        column = data->metaProperty.columnName().toLatin1();
    else
        column = QByteArray("_Qp_FK_") + data->metaProperty.name().toLatin1();

    if (newObject)
        data->owner->setProperty(column, Qp::Private::primaryKey(newObject.data()));
    else
        data->owner->setProperty(column, 0);
}
