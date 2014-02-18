#include "relation_hasone.h"
#include <QSharedData>

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"

class QpHasOneData : public QSharedData {
public:
    QpHasOneData() : QSharedData(),
        parent(nullptr),
        cleared(false)
    {}

    QSharedPointer<QObject> object;
    QpMetaProperty metaProperty;
    QObject *parent;
    bool cleared;
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
    if(data->object)
        return data->object;

    if(!data->cleared)
        data->object = QpRelationResolver::resolveToOneRelation(data->metaProperty.name(), data->parent);

    return data->object;
}

void QpHasOneBase::setObject(const QSharedPointer<QObject> object) const
{
    if(!object)
        data->cleared = true;

    if(data->object == object)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();

    QSharedPointer<QObject> previous = data->object;

    data->object = object;

    if(previous)
        reverse.write(previous.data(), objectVariant(QSharedPointer<QObject>()));

    data->object = object;

    if(object)
        reverse.write(object.data(), objectVariant(Qp::sharedFrom(data->parent)));
}
