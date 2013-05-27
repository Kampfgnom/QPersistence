#include "childobject.h"

#include "parentobject.h"

#include <QPersistence.h>

#include <QDebug>

ChildObject::ChildObject(QObject *parent) :
    QObject(parent),
    m_someInt(0)
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
    QSharedPointer<ParentObject> ref = m_parentObject.toStrongRef();

    if(!ref) {
        ref = Qp::resolveToOneRelation<ParentObject>("parentObject", this);
        m_parentObject = ref.toWeakRef();
    }

    return ref;
}

void ChildObject::setParentObject(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObject = parentObject.toWeakRef();
}

QSharedPointer<ParentObject> ChildObject::parentObject2() const
{
    QSharedPointer<ParentObject> ref = m_parentObject2.toStrongRef();

    if(!ref) {
        ref = Qp::resolveToOneRelation<ParentObject>("parentObject2", this);
        m_parentObject2 = ref.toWeakRef();
    }

    return ref;
}

void ChildObject::setParentObject2(const QSharedPointer<ParentObject> &parentObject)
{
    m_parentObject2 = parentObject.toWeakRef();
}
