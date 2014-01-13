#include "parentobject.h"

#include "childobject.h"

#include <QDebug>
#include <QPersistence.h>

ParentObject::ParentObject(QObject *parent) :
    QObject(parent),
    m_childObject("childObject", this),
    m_childObjects("childObjects", this)
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
    object->setParentObject(Qp::sharedFrom(this));
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

    object->setParentObject2(Qp::sharedFrom(this));
    m_childObjects.relate(object);
}
