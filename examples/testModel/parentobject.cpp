#include "parentobject.h"

#include "childobject.h"

#include <QDebug>
#include <QPersistence.h>

ParentObject::ParentObject(QObject *parent) :
    QObject(parent),
    m_childObjectOneToOne("childObjectOneToOne", this),
    m_childObjectsOneToMany("childObjectsOneToMany", this),
    m_childObjectsManyToMany("childObjectsManyToMany", this),
    m_counter(0),
    m_hasOne(QpRelation(&ParentObject::hasOne)),
    m_hasMany(QpRelation(&ParentObject::hasMany)),
    m_hasManyMany(QpRelation(&ParentObject::hasManyMany))
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
    if(child == previousChild)
        return;

    if(previousChild)
        previousChild->setParentObjectOneToOne(QSharedPointer<ParentObject>());

    if(!child) {
        m_childObjectOneToOne.clear();
        return;
    }

    QSharedPointer<ParentObject> previousParent = child->parentObjectOneToOne();
    if(previousParent)
        previousParent->setChildObjectOneToOne(QSharedPointer<ChildObject>());

    child->setParentObjectOneToOne(Qp::sharedFrom(this));
    m_childObjectOneToOne.relate(child);
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjectsOneToMany() const
{
    return m_childObjectsOneToMany.resolveList();
}

void ParentObject::addChildObjectOneToMany(QSharedPointer<ChildObject> child)
{
    if(!child)
        return;

    QSharedPointer<ParentObject> previousParent = child->parentObjectOneToMany();
    QSharedPointer<ParentObject> sharedThis = Qp::sharedFrom(this);

    if(previousParent == sharedThis)
        return;

    if(previousParent) {
        previousParent->removeChildObjectOneToMany(child);
    }

    child->setParentObjectOneToMany(sharedThis);
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

int ParentObject::counter() const
{
    return m_counter;
}

void ParentObject::increaseCounter()
{
    setCounter(counter() + 1);
}

QSharedPointer<ChildObject> ParentObject::hasOne() const
{
    return m_hasOne;
}

QList<QSharedPointer<ChildObject> > ParentObject::hasMany() const
{
    return m_hasMany;
}

void ParentObject::setHasOne(QSharedPointer<ChildObject> arg)
{
    m_hasOne = arg;
}

void ParentObject::setHasMany(QList<QSharedPointer<ChildObject> > arg)
{
    m_hasMany = arg;
}

QList<QSharedPointer<ChildObject> > ParentObject::hasManyMany() const
{
    return m_hasManyMany;
}

void ParentObject::setHasManyMany(QList<QSharedPointer<ChildObject> > arg)
{
    m_hasManyMany = arg;
}

void ParentObject::addHasManyMany(QSharedPointer<ChildObject> arg)
{
    m_hasManyMany.append(arg);
}

void ParentObject::removeHasManyMany(QSharedPointer<ChildObject> arg)
{
    m_hasManyMany.remove(arg);
}

void ParentObject::addHasMany(QSharedPointer<ChildObject> arg)
{
    m_hasMany.append(arg);
}

void ParentObject::removeHasMany(QSharedPointer<ChildObject> arg)
{
    m_hasMany.remove(arg);
}

void ParentObject::setCounter(int arg)
{
    m_counter = arg;
}

void ParentObject::setChildObjectsOneToMany(QList<QSharedPointer<ChildObject> > arg)
{
    if(m_childObjectsOneToMany.isResolved()) {
        foreach(QSharedPointer<ChildObject> child, childObjectsOneToMany()) {
            child->setParentObjectOneToMany(QSharedPointer<ParentObject>());
        }

        m_childObjectsOneToMany.clear();

        foreach(QSharedPointer<ChildObject> child,  arg) {
            QSharedPointer<ParentObject> previousParent = child->parentObjectOneToMany();
            QSharedPointer<ParentObject> sharedThis = Qp::sharedFrom(this);

            if(previousParent != sharedThis) {
                if(previousParent) {
                    previousParent->removeChildObjectOneToMany(child);
                }

                child->setParentObjectOneToMany(sharedThis);
            }
        }
    }

    m_childObjectsOneToMany.relate(arg);
}

void ParentObject::setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg)
{
    if(m_childObjectsManyToMany.isResolved()) {
        QSharedPointer<ParentObject> sharedThis = Qp::sharedFrom(this);
        foreach(QSharedPointer<ChildObject> child, childObjectsManyToMany()) {
            child->removeParentObjectManyToMany(sharedThis);
        }

        m_childObjectsManyToMany.clear();

        foreach(QSharedPointer<ChildObject> child,  arg) {
            child->addParentObjectManyToMany(sharedThis);
        }
    }

    m_childObjectsManyToMany.relate(arg);
}
