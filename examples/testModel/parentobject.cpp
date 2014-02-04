#include "parentobject.h"

#include "childobject.h"

#include <QDebug>
#include <QPersistence.h>

ParentObject::ParentObject(QObject *parent) :
    QObject(parent),
    m_childObjectOneToOne("childObjectOneToOne", this),
    m_childObjectsOneToMany("childObjectsOneToMany", this),
    m_childObjectsManyToMany("childObjectsManyToMany", this)
{
}

ParentObject::~ParentObject()
{
}

QString ParentObject::aString() const
{
    return m_astring;
}

void ParentObject::setAString(const QString &value)
{
    m_astring = value;
}

QSharedPointer<ChildObject> ParentObject::childObjectOneToOne() const
{
    return m_childObjectOneToOne.resolve();
}

void ParentObject::setChildObjectOneToOne(QSharedPointer<ChildObject> child)
{
    QSharedPointer<ChildObject> previousChild = childObjectOneToOne();
    if(previousChild)
        previousChild->setParentObjectOneToOne(QSharedPointer<ParentObject>());

    if(!child) {
        m_childObjectOneToOne.clear();
        return;
    }

    QSharedPointer<ParentObject> previousParent = child->parentObjectOneToOne();
    if(previousParent) {
        previousParent->setChildObjectOneToOne(QSharedPointer<ChildObject>());
    }

    child->setParentObjectOneToOne(Qp::sharedFrom(this));
    m_childObjectOneToOne.relate(child);
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjectsOneToMany() const
{
    return m_childObjectsOneToMany.resolveList();
}

void ParentObject::addChildObjectOneToMany(QSharedPointer<ChildObject> child)
{
    if (m_childObjectsOneToMany.contains(child))
        return;

    if(!child)
        return;

    QSharedPointer<ParentObject> previousParent = child->parentObjectOneToMany();
    if(previousParent) {
        previousParent->removeChildObjectOneToMany(child);
    }

    child->setParentObjectOneToMany(Qp::sharedFrom(this));
    m_childObjectsOneToMany.relate(child);
}

void ParentObject::removeChildObjectOneToMany(QSharedPointer<ChildObject> child)
{
    if(!child)
        return;

    if (!m_childObjectsOneToMany.contains(child))
        return;

    child->setParentObjectOneToMany(QSharedPointer<ParentObject>());
    m_childObjectsOneToMany.unrelate(child);
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjectsManyToMany() const
{
    return m_childObjectsManyToMany.resolveList();
}

void ParentObject::addChildObjectManyToMany(QSharedPointer<ChildObject> arg)
{
    if(!arg)
        return;

    if (m_childObjectsManyToMany.contains(arg))
        return;

    m_childObjectsManyToMany.relate(arg);
    arg->addParentObjectManyToMany(Qp::sharedFrom(this));
}

void ParentObject::removeChildObjectManyToMany(QSharedPointer<ChildObject> child)
{
    if(!child)
        return;

    if (!m_childObjectsManyToMany.contains(child))
        return;

    child->removeParentObjectManyToMany(Qp::sharedFrom(this));
    m_childObjectsManyToMany.unrelate(child);
}

void ParentObject::setChildObjectsOneToMany(QList<QSharedPointer<ChildObject> > arg)
{
    m_childObjectsOneToMany.relate(arg);
}

void ParentObject::setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg)
{
    m_childObjectsManyToMany.relate(arg);
}
