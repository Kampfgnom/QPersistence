#include "childobject.h"

#include "parentobject.h"

#include <QPersistence.h>

#include <QDebug>

ChildObject::ChildObject(QObject *parent) :
    QObject(parent),
    m_someInt(0),
    m_parentObject("parentObjectOneToOne", this),
    m_parentObject2("parentObjectOneToMany", this),
    m_parentObjectsManyToMany("parentObjectsManyToMany", this)
{
}

ChildObject::~ChildObject()
{
    qDebug() << Q_FUNC_INFO;
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
    return m_parentObject.resolve();
}

void ChildObject::setParentObjectOneToOne(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObject.relate(parentObject);
}

QSharedPointer<ParentObject> ChildObject::parentObjectOneToMany() const
{
    return m_parentObject2.resolve();
}

QList<QSharedPointer<ParentObject> > ChildObject::parentObjectsManyToMany() const
{
    return m_parentObjectsManyToMany.resolveList();
}

void ChildObject::setParentObjectsManyToMany(QList<QSharedPointer<ParentObject> > arg)
{
    m_parentObjectsManyToMany.clear();
    m_parentObjectsManyToMany.relate(arg);
}

void ChildObject::addParentObjectManyToMany(QSharedPointer<ParentObject> arg)
{
    m_parentObjectsManyToMany.relate(arg);
}

void ChildObject::setParentObjectOneToMany(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObject2.relate(parentObject);
}
