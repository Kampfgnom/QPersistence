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

void QpBelongsToOneBase::setObject(const QSharedPointer<QObject> object) const
{
    QSharedPointer<QObject> previous = this->object();
    if(previous == object)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();

    if(previous)
        reverse.write(previous.data(), objectVariant(QSharedPointer<QObject>()));

    data->object = object.toWeakRef();
}
