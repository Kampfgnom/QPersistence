#include "parentobject.h"

#include "childobject.h"

#include <QDebug>
#include <QPersistence.h>

#ifdef Q_OS_WIN
#include <time.h>
#endif

namespace TestNameSpace {

int ParentObject::NEXT_INDEX(-1);

ParentObject::ParentObject(QObject *parent) :
    QObject(parent),
    m_childObjectOneToOne(QpRelation(&ParentObject::childObjectOneToOne)),
    m_childObjectsOneToMany(QpRelation(&ParentObject::childObjectsOneToMany)),
    m_childObjectsManyToMany(QpRelation(&ParentObject::childObjectsManyToMany)),
    m_counter(0),
    m_hasOne(QpRelation(&ParentObject::hasOne)),
    m_hasMany(QpRelation(&ParentObject::hasMany)),
    m_hasManyMany(QpRelation(&ParentObject::hasManyMany)),
    m_testEnum(InitialValue),
    m_testOptions(InitialOption),
    m_indexed(0),
    m_customColumn(-1),
    m_calculatedIntDependency(0),
    m_calculatedInt(0),
    m_calculatedInt2(0),
    m_calculatedFromHasOne(0),
    m_calculatedFromHasMany(0),
    m_calculatedFromHasManyMany(0)
{
    if(NEXT_INDEX == -1)
        qsrand(static_cast<uint>(time(0)));

    NEXT_INDEX = qrand();
    m_indexed = NEXT_INDEX;
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

ParentObject::TestOptions ParentObject::testOptions() const
{
    return m_testOptions;
}

void ParentObject::setTestOptions(TestOptions arg)
{
    m_testOptions = arg;
}

int ParentObject::indexed() const
{
    return m_indexed;
}

int ParentObject::customColumn() const
{
    return m_customColumn;
}

int ParentObject::calculatedIntDependency() const
{
    return m_calculatedIntDependency;
}

int ParentObject::calculatedInt() const
{
    return m_calculatedInt;
}

int ParentObject::calculatedInt2() const
{
    return m_calculatedInt2;
}

int ParentObject::calculatedFromHasOne() const
{
    if(m_calculatedFromHasOne == 0)
        recalculateCalculatedFromHasOne();

    return m_calculatedFromHasOne;
}

int ParentObject::calculatedFromHasMany() const
{
    if(m_calculatedFromHasMany == 0)
        recalculateCalculatedFromHasMany();

    return m_calculatedFromHasMany;
}

int ParentObject::calculatedFromHasManyMany() const
{
    if(m_calculatedFromHasManyMany == 0)
        recalculateCalculatedFromHasManyMany();

    return m_calculatedFromHasManyMany;
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

void ParentObject::setIndexed(int arg)
{
    m_indexed = arg;
}

void ParentObject::setCustomColumn(int arg)
{
    m_customColumn = arg;
}

void ParentObject::setCalculatedIntDependency(int arg)
{
    if (m_calculatedIntDependency == arg)
        return;

    m_calculatedIntDependency = arg;
    emit calculatedIntDependencyChanged(arg);
}

void ParentObject::setCalculatedFromHasOne(int arg) const
{
    if (m_calculatedFromHasOne == arg)
        return;

    m_calculatedFromHasOne = arg;
    emit calculatedFromHasOneChanged(arg);
}

void ParentObject::recalculateCalculatedFromHasOne() const
{
    if(QSharedPointer<ChildObject> c = hasOne())
        setCalculatedFromHasOne(2 * c->calculatedIntDependency());
    else
        setCalculatedFromHasOne(0);
}

void ParentObject::setCalculatedFromHasMany(int arg) const
{
    if (m_calculatedFromHasMany == arg)
        return;

    m_calculatedFromHasMany = arg;
    emit calculatedFromHasManyChanged(arg);
}

void ParentObject::recalculateCalculatedFromHasMany() const
{
    int result = 0;
    foreach(QSharedPointer<ChildObject> child, hasMany()) {
        result += 3 * child->calculatedIntDependency();
    }
    setCalculatedFromHasMany(result);
}

void ParentObject::setCalculatedFromHasManyMany(int arg) const
{
    if (m_calculatedFromHasManyMany == arg)
        return;

    m_calculatedFromHasManyMany = arg;
    emit calculatedFromHasManyManyChanged(arg);
}

void ParentObject::recalculateCalculatedFromHasManyMany() const
{
    int result = 0;
    foreach(QSharedPointer<ChildObject> child, hasManyMany()) {
        result += 5 * child->calculatedIntDependency();
    }
    setCalculatedFromHasManyMany(result);
}

void ParentObject::setCalculatedInt2(int arg) const
{
    if (m_calculatedInt2 == arg)
        return;

    m_calculatedInt2 = arg;
    emit calculatedInt2Changed(arg);
}

void ParentObject::recalculateCalculatedInt() const
{
    setCalculatedInt(m_calculatedIntDependency * 5);
}

void ParentObject::setCalculatedInt(int arg) const
{
    if (m_calculatedInt == arg)
        return;

    m_calculatedInt = arg;
    emit calculatedIntChanged(arg);
}

void ParentObject::recalculateCalculatedInt2() const
{
    setCalculatedInt2(m_calculatedInt * 23);
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
