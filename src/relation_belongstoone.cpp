#include "relation_belongstoone.h"
#include <QSharedData>

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"

class QpBelongsToOneData : public QSharedData {
public:
    QpBelongsToOneData() : QSharedData(),
        parent(nullptr)
    {}

    QWeakPointer<QObject> object;
    QpMetaProperty metaProperty;
    QObject *parent;
};

QpBelongsToOneBase::QpBelongsToOneBase(const QString &name, QObject *parent) :
    data(new QpBelongsToOneData)
{
    data->parent = parent;
    QString n = name.mid(name.lastIndexOf("::") + 2);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(n);
}

QpBelongsToOneBase::~QpBelongsToOneBase()
{
}

QSharedPointer<QObject> QpBelongsToOneBase::object() const
{
    QSharedPointer<QObject> object = data->object.toStrongRef();
    if(object)
        return object;

    object = QpRelationResolver::resolveToOneRelation(data->metaProperty.name(), data->parent);
    data->object = object.toWeakRef();
    return object;
}

void QpBelongsToOneBase::setObject(const QSharedPointer<QObject> newObject) const
{
    QSharedPointer<QObject> previousObject = this->object();
    if(previousObject == newObject)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    data->object = newObject.toWeakRef();

    if(previousObject){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(previousObject.data(), objectVariant(QSharedPointer<QObject>()));
        }
        else {
            invokeMethod(reverse.removeObjectMethod(), previousObject.data(), Qp::sharedFrom(data->parent));
        }
    }

    // Set again, because it may happen, that resetting the previousObjects relation has also reset this value.
    data->object = newObject.toWeakRef();
}
