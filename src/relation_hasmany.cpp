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
    if(!data->resolved)
        return data->objects;

    data->objects = QpRelationResolver::resolveToManyRelation(data->metaProperty.name(), data->parent);
    data->resolved = true;
    return data->objects;
}

void QpHasManyBase::append(QSharedPointer<QObject> object)
{
    objects(); // resolve
    Q_ASSERT(!data->objects.contains(object));

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    QSharedPointer<QObject> sharedParent = Qp::sharedFrom(data->parent);
    if(object){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(object.data(), objectVariant(sharedParent));
        }
        else {
            invokeMethod(reverse.addObjectMethod(), object.data(), sharedParent);
        }
    }
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
    QSharedPointer<QObject> sharedParent = Qp::sharedFrom(data->parent);
    if(object){
        if(reverse.isToOneRelationProperty()) {
            reverse.write(object.data(), objectVariant(QSharedPointer<QObject>()));
        }
        else {
            invokeMethod(reverse.removeObjectMethod(), object.data(), sharedParent);
        }
    }
}

void QpHasManyBase::setObjects(const QList<QSharedPointer<QObject>> objects) const
{
    data->objects = objects;
    data->resolved = true;
}
