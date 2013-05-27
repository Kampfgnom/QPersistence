#include "parentobject.h"

#include "childobject.h"

#include <QDebug>
#include <QPersistence.h>

ParentObject::ParentObject(QObject *parent) :
    QObject(parent)
{
}

ParentObject::~ParentObject()
{
    qDebug() << Q_FUNC_INFO;
}

QString ParentObject::aString() const
{
    return m_astring;
}

void ParentObject::setAString(const QString &value)
{
    m_astring = value;
}

QSharedPointer<ChildObject> ParentObject::childObject() const
{
    return m_childObject;
}

void ParentObject::setChildObject(QSharedPointer<ChildObject> object)
{
    m_childObject = object;

    if(object) {
        QSharedPointer<ParentObject> sharedThis = Qp::sharedFrom<ParentObject>(this);
        object->setParentObject(sharedThis);
    }
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjects() const
{
    return m_childObjects;
}

void ParentObject::addChildObject(QSharedPointer<ChildObject> object)
{
    if(!object || m_childObjects.contains(object))
        return;

    m_childObjects.append(object);
    QSharedPointer<ParentObject> sharedThis = Qp::sharedFrom<ParentObject>(this);
    object->setParentObject2(sharedThis);
}
