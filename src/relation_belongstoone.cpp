#include "relation_belongstoone.h"

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpBelongsToOneData : public QSharedData {
public:
    QpBelongsToOneData() : QSharedData(),
        cleared(false),
        parent(nullptr)
    {}

    bool cleared;
    QWeakPointer<QObject> object;
    QpMetaProperty metaProperty;
    QObject *parent;
};

QpBelongsToOneBase::QpBelongsToOneBase(const QString &name, QObject *parent) :
    data(new QpBelongsToOneData)
{
    data->parent = parent;
    int classNameEndIndex = name.lastIndexOf("::");
    QString n = name;
    if(classNameEndIndex >= 0)
        n = name.mid(classNameEndIndex + 2);

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

    if(data->cleared)
        return QSharedPointer<QObject>();

    object = QpRelationResolver::resolveToOneRelation(data->metaProperty.name(), data->parent);
    data->object = object.toWeakRef();
    return object;
}

void QpBelongsToOneBase::setObject(const QSharedPointer<QObject> newObject) const
{
    QSharedPointer<QObject> previousObject = object();
    data->cleared = newObject.isNull();
    if(previousObject == newObject)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    data->object = newObject.toWeakRef();
    QSharedPointer<QObject> shared = Qp::sharedFrom(data->parent);

    if(previousObject){
        if(reverse.isToOneRelationProperty()) {
            QString className = data->metaProperty.metaObject().className();
            reverse.write(previousObject.data(), Qp::Private::variantCast(QSharedPointer<QObject>(), className));
        }
        else {
            QVariant wrapper = Qp::Private::variantCast(shared);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.removeObjectMethod(reverse);

            Q_ASSERT(method.invoke(previousObject.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }

    if(newObject){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(newObject.data(), Qp::Private::variantCast(shared));
        }
        else {
            QVariant wrapper = Qp::Private::variantCast(shared);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.addObjectMethod(reverse);

            Q_ASSERT(method.invoke(newObject.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }

    // Set again, because it may happen, that resetting the previousObjects relation has also reset this value.
    data->object = newObject.toWeakRef();
}
