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
    QString className = data->metaProperty.metaObject().className();
    data->object = newObject;

    QSharedPointer<QObject> sharedParent = Qp::sharedFrom(data->parent);
    if(previousObject) {
        if(reverse.isToOneRelationProperty()) {
            reverse.write(previousObject.data(), Qp::Private::variantCast(QSharedPointer<QObject>(), className));
        }
        else {
            QVariant wrapper = Qp::Private::variantCast(sharedParent);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.removeObjectMethod(reverse);

            Q_ASSERT(method.invoke(previousObject.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }

    if(newObject){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(newObject.data(), Qp::Private::variantCast(sharedParent));
        }
        else {
            QVariant wrapper = Qp::Private::variantCast(sharedParent);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.addObjectMethod(reverse);

            Q_ASSERT(method.invoke(newObject.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }

    // Set again, because it may (will) happen, that setting the reverse relations has also changed this value.
    data->object = newObject;
}
