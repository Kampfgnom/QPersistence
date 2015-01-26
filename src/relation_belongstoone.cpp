#include "relation_belongstoone.h"

#include "metaproperty.h"
#include "propertydependencieshelper.h"
#include "qpersistence.h"
#include "relationresolver.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


/******************************************************************************
 * QpBelongsToOneData
 */
class QpBelongsToOneData : public QSharedData {
public:
    QpBelongsToOneData() :
        QSharedData(),
        cleared(false),
        owner(nullptr)
    {
    }

    bool cleared;
    QWeakPointer<QObject> object;
    QpMetaProperty metaProperty;
    QObject *owner;
};


/******************************************************************************
 * QpBelongsToOneBase
 */
QpBelongsToOneBase::QpBelongsToOneBase(const QString &name, QObject *parent) :
    data(new QpBelongsToOneData)
{
    data->owner = parent;
    QString propertyName = QpMetaProperty::nameFromMaybeQualifiedName(name);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(propertyName);
}

QpBelongsToOneBase::~QpBelongsToOneBase()
{
}

bool QpBelongsToOneBase::operator ==(const QSharedPointer<QObject> &object) const
{
    return data->object == object;
}

QSharedPointer<QObject> QpBelongsToOneBase::object() const
{
    if (Qp::Private::primaryKey(data->owner) == 0)
        return QSharedPointer<QObject>();

    QSharedPointer<QObject> object = data->object.toStrongRef();
    if (object)
        return object;

    if (data->cleared)
        return QSharedPointer<QObject>();

    object = QpRelationResolver::resolveToOneRelation(data->metaProperty.name(), data->owner);
    data->object = object.toWeakRef();

    QpStorage::forObject(data->owner)->propertyDependenciesHelper()->initDependencies(data->owner, object, data->metaProperty);

    return object;
}

void QpBelongsToOneBase::setObject(const QSharedPointer<QObject> newObject) const
{
    QSharedPointer<QObject> previousObject = object();
    data->cleared = newObject.isNull();
    if (previousObject == newObject)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    data->object = newObject.toWeakRef();
    QSharedPointer<QObject> sharedOwner = Qp::sharedFrom(data->owner);
    QpPropertyDependenciesHelper *dependeciesHelper = QpStorage::forObject(data->owner)->propertyDependenciesHelper();

    if (previousObject) {
        reverse.remove(previousObject, sharedOwner);
        dependeciesHelper->removeDependencies(data->owner, previousObject, data->metaProperty);
    }

    if (newObject) {
        reverse.add(newObject, sharedOwner);
        dependeciesHelper->initDependencies(data->owner, newObject, data->metaProperty);
    }

    // Set again, because it may happen, that resetting the previousObjects relation has also reset this value.
    data->object = newObject.toWeakRef();

    // Set dynamic property for relation
    QByteArray column;
    if (data->metaProperty.hasTableForeignKey())
        column = data->metaProperty.columnName().toLatin1();
    else
        column = QByteArray("_Qp_FK_") + data->metaProperty.name().toLatin1();

    if (newObject)
        sharedOwner->setProperty(column, Qp::Private::primaryKey(newObject.data()));
    else
        sharedOwner->setProperty(column, 0);
}
