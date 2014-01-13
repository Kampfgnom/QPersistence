#include "childobject.h"

#include "parentobject.h"

#include <QPersistence.h>

#include <QDebug>

ChildObject::ChildObject(QObject *parent) :
    QObject(parent),
    m_someInt(0),
    m_parentObject("parentObject", this),
    m_parentObject2("parentObject2", this)
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

QSharedPointer<ParentObject> ChildObject::parentObject() const
{
    return m_parentObject.resolve();
}

void ChildObject::setParentObject(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObject.relate(parentObject);
}

QSharedPointer<ParentObject> ChildObject::parentObject2() const
{
    return m_parentObject2.resolve();
}

void ChildObject::setParentObject2(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObject2.relate(parentObject);
}
