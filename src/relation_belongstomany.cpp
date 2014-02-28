#include "relation_belongstomany.h"

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


class QpBelongsToManyData : public QSharedData {
public:
    QpBelongsToManyData() : QSharedData(),
        resolved(false),
        parent(nullptr)
    {}

    bool resolved;
    QList<QWeakPointer<QObject>> objects;
    QpMetaProperty metaProperty;
    QObject *parent;
};

QpBelongsToManyBase::QpBelongsToManyBase(const QString &name, QObject *parent) :
    data(new QpBelongsToManyData)
{
    data->parent = parent;
    QString n = name.mid(name.lastIndexOf("::") + 2);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(n);
}

QpBelongsToManyBase::~QpBelongsToManyBase()
{
}

QList<QSharedPointer<QObject> > QpBelongsToManyBase::objects() const
{
    if(data->resolved) {
        bool ok = false;
        QList<QSharedPointer<QObject> > objs = Qp::Private::makeListStrong(data->objects, &ok);
        if(ok)
            return objs;
    }

    QList<QSharedPointer<QObject> > objs = QpRelationResolver::resolveToManyRelation(data->metaProperty.name(), data->parent);
    data->objects = Qp::Private::makeListWeak(objs);
    data->resolved = true;
    return objs;
}

void QpBelongsToManyBase::add(QSharedPointer<QObject> object)
{
    QList<QSharedPointer<QObject>> obj = objects(); Q_UNUSED(obj); // resolve and keep a strong ref, while we're working here

    QWeakPointer<QObject> weakRef = object.toWeakRef();
    if(data->objects.contains(weakRef))
        return;

    data->objects.append(weakRef);

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    QSharedPointer<QObject> sharedParent = Qp::sharedFrom(data->parent);

    if(object){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(object.data(), Qp::Private::variantCast(sharedParent));
        }
        else {
            QString className = data->metaProperty.metaObject().className();

            QSharedPointer<QObject> shared = Qp::sharedFrom(data->parent);
            QVariant wrapper = Qp::Private::variantCast(shared, className);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.addObjectMethod(reverse);

            Q_ASSERT(method.invoke(object.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }
}

void QpBelongsToManyBase::remove(QSharedPointer<QObject> object)
{
    QList<QSharedPointer<QObject>> obj = objects(); Q_UNUSED(obj); // resolve and keep a strong ref, while we're working here

    int removeCount = data->objects.removeAll(object.toWeakRef());
    Q_ASSERT(removeCount <= 1);

    if(removeCount == 0)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();

    if(object){
        if(reverse.isToOneRelationProperty()) {
            QString className = data->metaProperty.metaObject().className();
            reverse.write(object.data(), Qp::Private::variantCast(QSharedPointer<QObject>(), className));
        }
        else {

            QSharedPointer<QObject> shared = Qp::sharedFrom(data->parent);
            QVariant wrapper = Qp::Private::variantCast(shared);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.removeObjectMethod(reverse);

            Q_ASSERT(method.invoke(object.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }
}

void QpBelongsToManyBase::setObjects(const QList<QSharedPointer<QObject>> objects) const
{
    data->objects = Qp::Private::makeListWeak(objects);
    data->resolved = true;
}
