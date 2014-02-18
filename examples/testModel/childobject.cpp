#include "childobject.h"

#include "parentobject.h"

#include <QPersistence.h>

#include <QDebug>

ChildObject::ChildObject(QObject *parent) :
    QObject(parent),
    m_someInt(0),
    m_parentObjectOneToOne("parentObjectOneToOne", this),
    m_parentObjectOneToMany("parentObjectOneToMany", this),
    m_parentObjectsManyToMany("parentObjectsManyToMany", this),
    m_belongsToOne(QpRelation(&ChildObject::belongsToOne)),
    m_belongsToOneMany(QpRelation(&ChildObject::belongsToOneMany))
{
}

ChildObject::~ChildObject()
{
}

int ChildObject::someInt() const
{
    return m_someInt;
}

void ChildObject::setSomeInt(int arg)
{
    m_someInt = arg;
}

QSharedPointer<ParentObject> ChildObject::parentObjectOneToOne() const
{
    return m_parentObjectOneToOne.resolve();
}

void ChildObject::setParentObjectOneToOne(const QSharedPointer<ParentObject> &parentObject)
{
    QSharedPointer<ParentObject> previousParent = parentObjectOneToOne();
    if(parentObject == previousParent)
        return;

    m_parentObjectOneToOne.relate(parentObject);

    if(previousParent)
        previousParent->setChildObjectOneToOne(QSharedPointer<ChildObject>());
}

QSharedPointer<ParentObject> ChildObject::parentObjectOneToMany() const
{
    return m_parentObjectOneToMany.resolve();
}

QList<QSharedPointer<ParentObject> > ChildObject::parentObjectsManyToMany() const
{
    return m_parentObjectsManyToMany.resolveList();
}

QSharedPointer<ParentObject> ChildObject::belongsToOne() const
{
    return m_belongsToOne;
}

QSharedPointer<ParentObject> ChildObject::belongsToOneMany() const
{
    return m_belongsToOneMany;
}

void ChildObject::setBelongsToOne(QSharedPointer<ParentObject> arg)
{
    m_belongsToOne = arg;
}

void ChildObject::setBelongsToOneMany(QSharedPointer<ParentObject> arg)
{
    m_belongsToOneMany = arg;
}

void ChildObject::setParentObjectsManyToMany(QList<QSharedPointer<ParentObject> > arg)
{
    if(m_parentObjectsManyToMany.isResolved()) {
        QSharedPointer<ChildObject> sharedThis = Qp::sharedFrom(this);
        foreach(QSharedPointer<ParentObject> parent, parentObjectsManyToMany()) {
            parent->removeChildObjectManyToMany(sharedThis);
        }

        m_parentObjectsManyToMany.clear();

        foreach(QSharedPointer<ParentObject> parent,  arg) {
            parent->addChildObjectManyToMany(sharedThis);
        }
    }

    m_parentObjectsManyToMany.relate(arg);
}

void ChildObject::addParentObjectManyToMany(QSharedPointer<ParentObject> arg)
{
    m_parentObjectsManyToMany.relate(arg);
}

void ChildObject::removeParentObjectManyToMany(QSharedPointer<ParentObject> arg)
{
    m_parentObjectsManyToMany.unrelate(arg);
}

void ChildObject::setParentObjectOneToMany(const QSharedPointer<ParentObject> &parentObject)
{
    QSharedPointer<ParentObject> previousParent = parentObjectOneToMany();
    if(parentObject == previousParent)
        return;

    m_parentObjectOneToMany.relate(parentObject);

    if(previousParent)
        previousParent->removeChildObjectOneToMany(Qp::sharedFrom(this));
}
