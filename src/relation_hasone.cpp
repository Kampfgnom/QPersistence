#include "relation_hasone.h"
#include <QSharedData>

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"

class QpHasOneData : public QSharedData {
public:
    QpHasOneData() : QSharedData(),
        parent(nullptr),
        resolved(false)
    {}

    QSharedPointer<QObject> object;
    QpMetaProperty metaProperty;
    QObject *parent;
    bool resolved;
};

QpHasOneBase::QpHasOneBase(const QString &name, QObject *parent) :
    data(new QpHasOneData)
{
    data->parent = parent;
    QString n = name.mid(name.lastIndexOf("::") + 2);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(n);
}

QpHasOneBase::~QpHasOneBase()
{
}

QSharedPointer<QObject> QpHasOneBase::object() const
{
    if(data->resolved)
        return data->object;

    data->object = QpRelationResolver::resolveToOneRelation(data->metaProperty.name(), data->parent);
    data->resolved = true;

    return data->object;
}

void QpHasOneBase::setObject(const QSharedPointer<QObject> newObject) const
{
    QSharedPointer<QObject> previousObject = object();

    if(previousObject == newObject)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    data->object = newObject;

    QSharedPointer<QObject> sharedParent = Qp::sharedFrom(data->parent);
    if(previousObject) {
        if(reverse.isToOneRelationProperty()) {
            reverse.write(previousObject.data(), objectVariant(QSharedPointer<QObject>()));
        }
        else {
            invokeMethod(reverse.removeObjectMethod(), previousObject.data(), sharedParent);
        }
    }

    if(newObject){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(newObject.data(), objectVariant(sharedParent));
        }
        else {
            invokeMethod(reverse.addObjectMethod(), newObject.data(), sharedParent);
        }
    }

    // Set again, because it may happen, that setting the reverse relations has also changed this value.
    data->object = newObject;
}
