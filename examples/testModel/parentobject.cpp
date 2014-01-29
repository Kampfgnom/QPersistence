#include "parentobject.h"

#include "childobject.h"

#include <QDebug>
#include <QPersistence.h>

ParentObject::ParentObject(QObject *parent) :
    QObject(parent),
    m_childObject("childObject", this),
    m_childObjects("childObjects", this),
    m_childObjectsManyToMany("childObjectsManyToMany", this)
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
    return m_childObject.resolve();
}

void ParentObject::setChildObject(QSharedPointer<ChildObject> object)
{
    if(childObject())
        childObject()->setParentObjectOneToOne(QSharedPointer<ParentObject>());

    if(object)
        object->setParentObjectOneToOne(Qp::sharedFrom(this));

    m_childObject.relate(object);
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjects() const
{
    return m_childObjects.resolveList();
}

void ParentObject::addChildObject(QSharedPointer<ChildObject> object)
{
    if (m_childObjects.contains(object))
        return;

    if(object)
        object->setParentObjectOneToMany(Qp::sharedFrom(this));

    m_childObjects.relate(object);
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjectsManyToMany() const
{
    return m_childObjectsManyToMany.resolveList();
}

void ParentObject::addChildObjectManyToMany(QSharedPointer<ChildObject> arg)
{
    if (m_childObjectsManyToMany.contains(arg))
        return;

    m_childObjectsManyToMany.relate(arg);

    if(arg)
        arg->addParentObjectManyToMany(Qp::sharedFrom(this));
}

void ParentObject::setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg)
{
    m_childObjectsManyToMany.relate(arg);
}
