#include "relation_hasmany.h"
#include <QSharedData>

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"

class QpHasManyData : public QSharedData {
public:
    QpHasManyData() : QSharedData(),
        resolved(false),
        parent(nullptr)
    {}

    bool resolved;
    QList<QSharedPointer<QObject>> objects;
    QpMetaProperty metaProperty;
    QObject *parent;
};

QpHasManyBase::QpHasManyBase(const QString &name, QObject *parent) :
    data(new QpHasManyData)
{
    data->parent = parent;
    QString n = name.mid(name.lastIndexOf("::") + 2);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(n);
}

QpHasManyBase::~QpHasManyBase()
{
}

QList<QSharedPointer<QObject> > QpHasManyBase::objects() const
{
    if(data->resolved)
        return data->objects;

    data->objects = QpRelationResolver::resolveToManyRelation(data->metaProperty.name(), data->parent);
    data->resolved = true;
    return data->objects;
}

void QpHasManyBase::add(QSharedPointer<QObject> object)
{
    objects(); // resolve

    if(data->objects.contains(object))
        return;

    data->objects.append(object);

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    QSharedPointer<QObject> sharedParent = Qp::sharedFrom(data->parent);
    QString className = data->metaProperty.metaObject().className();

    if(object){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(object.data(), Qp::Private::variantCast(sharedParent, className));
        }
        else {

            QSharedPointer<QObject> shared = Qp::sharedFrom(data->parent);
            QVariant wrapper = Qp::Private::variantCast(shared, className);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.addObjectMethod(reverse);

            Q_ASSERT(method.invoke(object.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }

    if(!data->objects.contains(object))
        data->objects.append(object);
}

void QpHasManyBase::remove(QSharedPointer<QObject> object)
{
    objects(); // resolve
    int removeCount = data->objects.removeAll(object);
    Q_ASSERT(removeCount <= 1);

    if(removeCount == 0)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    QString className = data->metaProperty.metaObject().className();

    if(object){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(object.data(), Qp::Private::variantCast(QSharedPointer<QObject>(), className));
        }
        else {
            QSharedPointer<QObject> shared = Qp::sharedFrom(data->parent);
            QVariant wrapper = Qp::Private::variantCast(shared, className);

            QpMetaObject reverseObject = reverse.metaObject();
            QMetaMethod method = reverseObject.removeObjectMethod(reverse);

            Q_ASSERT(method.invoke(object.data(), Qt::DirectConnection,
                                   QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data())));
        }
    }
}

void QpHasManyBase::setObjects(const QList<QSharedPointer<QObject>> objects) const
{
    data->objects = objects;
    data->resolved = true;
}
