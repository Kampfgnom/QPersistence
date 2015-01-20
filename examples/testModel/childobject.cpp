#include "childobject.h"

#include "parentobject.h"

#include <QPersistence.h>

#include <QDebug>

namespace TestNameSpace {

ChildObject::ChildObject(QObject *parent) :
    QObject(parent),
    m_someInt(0),
    m_parentObjectOneToOne(QpRelation(&ChildObject::parentObjectOneToOne)),
    m_parentObjectOneToMany(QpRelation(&ChildObject::parentObjectOneToMany)),
    m_parentObjectsManyToMany(QpRelation(&ChildObject::parentObjectsManyToMany)),
    m_belongsToOne(QpRelation(&ChildObject::belongsToOne)),
    m_belongsToOneMany(QpRelation(&ChildObject::belongsToOneMany)),
    m_belongsToManyMany(QpRelation(&ChildObject::belongsToManyMany)),
    m_calculatedIntDependency(0),
    m_calculatedFromToOne(0),
    m_calculatedFromToMany(0),
    m_calculatedFromManyToMany(0)
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

int ChildObject::calculatedIntDependency() const
{
    return m_calculatedIntDependency;
}

int ChildObject::calculatedFromToOne() const
{
    if(m_calculatedFromToOne == 0)
        recalculateCalculatedFromToOne();

    return m_calculatedFromToOne;
}

int ChildObject::calculatedFromToMany() const
{
    if(m_calculatedFromToMany == 0)
        recalculateCalculatedFromToMany();

    return m_calculatedFromToMany;
}

int ChildObject::calculatedFromManyToMany() const
{
    if(m_calculatedFromManyToMany == 0)
        recalculateCalculatedFromManyToMany();

    return m_calculatedFromManyToMany;
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

void ChildObject::setCalculatedIntDependency(int arg)
{
    if (m_calculatedIntDependency == arg)
        return;

    m_calculatedIntDependency = arg;
    emit calculatedIntDependencyChanged(arg);
}

void ChildObject::setCalculatedFromToOne(int arg) const
{
    if (m_calculatedFromToOne == arg)
        return;

    m_calculatedFromToOne = arg;
    emit calculatedFromToOneChanged(arg);
}

void ChildObject::recalculateCalculatedFromToOne() const
{
    if(QSharedPointer<ParentObject> p = belongsToOne())
        setCalculatedFromToOne(p->calculatedIntDependency() * 17);
    else
        setCalculatedFromToOne(0);
}

void ChildObject::setCalculatedFromToMany(int arg) const
{
    if (m_calculatedFromToMany == arg)
        return;

    m_calculatedFromToMany = arg;
    emit calculatedFromToManyChanged(arg);
}

void ChildObject::recalculateCalculatedFromToMany() const
{
    if(QSharedPointer<ParentObject> p = belongsToOneMany())
        setCalculatedFromToMany(p->calculatedIntDependency() * 23);
    else
        setCalculatedFromToMany(0);
}

void ChildObject::setCalculatedFromManyToMany(int arg) const
{
    if (m_calculatedFromManyToMany == arg)
        return;

    m_calculatedFromManyToMany = arg;
    emit calculatedFromManyToManyChanged(arg);
}

void ChildObject::recalculateCalculatedFromManyToMany() const
{
    int result = 0;
    foreach(QSharedPointer<ParentObject> p, belongsToManyMany()) {
        result += p->calculatedIntDependency() * 29;
    }
    setCalculatedFromManyToMany(result);
}

void ChildObject::setParentObjectOneToMany(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObjectOneToMany = parentObject;
}

}
