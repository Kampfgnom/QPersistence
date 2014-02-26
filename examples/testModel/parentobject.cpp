#include "parentobject.h"

#include "childobject.h"

#include <QDebug>
#include <QPersistence.h>

namespace TestNameSpace {

ParentObject::ParentObject(QObject *parent) :
    QObject(parent),
    m_childObjectOneToOne(QpRelation(&ParentObject::childObjectOneToOne)),
    m_childObjectsOneToMany(QpRelation(&ParentObject::childObjectsOneToMany)),
    m_childObjectsManyToMany(QpRelation(&ParentObject::childObjectsManyToMany)),
    m_counter(0),
    m_hasOne(QpRelation(&ParentObject::hasOne)),
    m_hasMany(QpRelation(&ParentObject::hasMany)),
    m_hasManyMany(QpRelation(&ParentObject::hasManyMany)),
    m_testEnum(InitialValue)
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
    return m_childObjectOneToOne;
}

void ParentObject::setChildObjectOneToOne(QSharedPointer<ChildObject> child)
{
    m_childObjectOneToOne = child;
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjectsOneToMany() const
{
    return m_childObjectsOneToMany;
}

void ParentObject::addChildObjectsOneToMany(QSharedPointer<ChildObject> child)
{
    m_childObjectsOneToMany.add(child);
}


void ParentObject::removeChildObjectsOneToMany(QSharedPointer<ChildObject> child)
{
    m_childObjectsOneToMany.remove(child);
}

QList<QSharedPointer<ChildObject> > ParentObject::childObjectsManyToMany() const
{
    return m_childObjectsManyToMany;
}

void ParentObject::addChildObjectsManyToMany(QSharedPointer<ChildObject> arg)
{
    m_childObjectsManyToMany.add(arg);
}

void ParentObject::removeChildObjectsManyToMany(QSharedPointer<ChildObject> child)
{
    m_childObjectsManyToMany.remove(child);
}

int ParentObject::counter() const
{
    return m_counter;
}

void ParentObject::increaseCounter()
{
    setCounter(counter() + 1);
}

QDateTime ParentObject::date() const
{
    return m_date;
}

void ParentObject::setDate(QDateTime arg)
{
    m_date = arg;
}

ParentObject::TestEnum ParentObject::testEnum() const
{
    return m_testEnum;
}

void ParentObject::setTestEnum(ParentObject::TestEnum arg)
{
    m_testEnum = arg;
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
    m_hasManyMany.add(arg);
}

void ParentObject::removeHasManyMany(QSharedPointer<ChildObject> arg)
{
    m_hasManyMany.remove(arg);
}

void ParentObject::addHasMany(QSharedPointer<ChildObject> arg)
{
    m_hasMany.add(arg);
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
    m_childObjectsOneToMany = arg;
}

void ParentObject::setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg)
{
    m_childObjectsManyToMany = arg;
}

}
