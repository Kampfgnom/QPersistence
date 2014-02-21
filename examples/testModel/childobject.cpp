#include "childobject.h"

#include "parentobject.h"

#include <QPersistence.h>

#include <QDebug>

ChildObject::ChildObject(QObject *parent) :
    QObject(parent),
    m_someInt(0),
    m_parentObjectOneToOne(QpRelation(&ChildObject::parentObjectOneToOne)),
    m_parentObjectOneToMany(QpRelation(&ChildObject::parentObjectOneToMany)),
    m_parentObjectsManyToMany(QpRelation(&ChildObject::parentObjectsManyToMany)),
    m_belongsToOne(QpRelation(&ChildObject::belongsToOne)),
    m_belongsToOneMany(QpRelation(&ChildObject::belongsToOneMany)),
    m_belongsToManyMany(QpRelation(&ChildObject::belongsToManyMany))
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
    return m_parentObjectOneToOne;
}

void ChildObject::setParentObjectOneToOne(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObjectOneToOne = parentObject;
}

QSharedPointer<ParentObject> ChildObject::parentObjectOneToMany() const
{
    return m_parentObjectOneToMany;
}

QList<QSharedPointer<ParentObject> > ChildObject::parentObjectsManyToMany() const
{
    return m_parentObjectsManyToMany;
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

QList<QSharedPointer<ParentObject> > ChildObject::belongsToManyMany() const
{
    return m_belongsToManyMany;
}

void ChildObject::setBelongsToManyMany(QList<QSharedPointer<ParentObject> > arg)
{
    m_belongsToManyMany = arg;
}

void ChildObject::addBelongsToManyMany(QSharedPointer<ParentObject> arg)
{
    m_belongsToManyMany.add(arg);
}

void ChildObject::removeBelongsToManyMany(QSharedPointer<ParentObject> arg)
{
    m_belongsToManyMany.remove(arg);
}

void ChildObject::setParentObjectsManyToMany(QList<QSharedPointer<ParentObject> > arg)
{
    m_parentObjectsManyToMany = arg;
}

void ChildObject::addParentObjectsManyToMany(QSharedPointer<ParentObject> arg)
{
    m_parentObjectsManyToMany.add(arg);
}

void ChildObject::removeParentObjectsManyToMany(QSharedPointer<ParentObject> arg)
{
    m_parentObjectsManyToMany.remove(arg);
}

void ChildObject::setParentObjectOneToMany(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObjectOneToMany = parentObject;
}
