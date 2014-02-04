#include "relations.h"
#include <QSharedData>

#include "metaproperty.h"
#include "qpersistence.h"
#include "relationresolver.h"

template<class T>
class QpWeakRelationData : public QSharedData {
public:
    QpWeakRelationData();

    QString name;
    QObject *parent;
    mutable bool resolved;
    mutable bool cleared;
    mutable QList<QWeakPointer<T> > relatedList;
    mutable QWeakPointer<T> related;
    QpMetaProperty metaProperty;
    QpMetaProperty::Cardinality cardinality;

    QList<QSharedPointer<T> > resolveFromDatabase() const;
    bool isToMany() const;
};

template<class T>
QpWeakRelationData<T>::QpWeakRelationData() :
    QSharedData()
{
}

template<class T>
bool QpWeakRelationData<T>::isToMany() const
{
    return cardinality == QpMetaProperty::OneToManyCardinality
            || cardinality == QpMetaProperty::ManyToManyCardinality;
}

template<class T>
QList<QSharedPointer<T> > QpWeakRelationData<T>::resolveFromDatabase() const
{
    QList<QSharedPointer<T> > resolvedList = Qp::castList<T>(QpRelationResolver::resolveRelation(name, parent));
    resolved = true;
    if (isToMany()) {
        relatedList.append(Qp::Private::makeListWeak<T>(resolvedList));
    }

    if (resolvedList.isEmpty())
        return QList<QSharedPointer<T> >();
    related = resolvedList.first();
    return resolvedList;
}

template<class T>
QpWeakRelation<T>::QpWeakRelation(const QString &name, QObject *parent) :
    data(new QpWeakRelationData<T>)
{
    data->name = name;
    data->parent = parent;
    data->resolved = false;
    data->cleared = false;
    data->metaProperty = QpMetaObject::forClassName(parent->metaObject()->className()).metaProperty(name);
    data->cardinality = data->metaProperty.cardinality();
}

template<class T>
QpWeakRelation<T>::QpWeakRelation(const QpWeakRelation &rhs) : data(rhs.data)
{
}

template<class T>
QpWeakRelation<T> &QpWeakRelation<T>::operator=(const QpWeakRelation<T> &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

template<class T>
QpWeakRelation<T>::~QpWeakRelation()
{
}

template<class T>
QList<QSharedPointer<T> > QpWeakRelation<T>::resolveList() const
{
    if(data->cleared)
        return QList<QSharedPointer<T> >();

    bool ok;
    QList<QSharedPointer<T> > result = Qp::Private::makeListStrong<T>(data->relatedList, &ok);
    if (!data->resolved || !ok)
        result = data->resolveFromDatabase();

    return result;
}

template<class T>
QSharedPointer<T> QpWeakRelation<T>::resolve() const
{
    if(data->cleared)
        return QSharedPointer<T>();

    QSharedPointer<T> strong = data->related.toStrongRef();
    if (!data->resolved || !strong) {
        QList<QSharedPointer<T> > list = data->resolveFromDatabase();
        if (list.isEmpty())
            return QSharedPointer<T>();

        Q_ASSERT(list.size() == 1);
        strong = list.first();
    }

    return strong;
}

template<class T>
bool QpWeakRelation<T>::contains(QSharedPointer<T> object) const
{
    return resolveList().contains(object);
}

template<class T>
void QpWeakRelation<T>::relate(QSharedPointer<T> related)
{
    if(!related) {
        clear();
        return;
    }

    data->cleared = false;

    if (data->isToMany()) {
        if (!data->resolved)
            data->resolveFromDatabase();

        data->relatedList.append(related.toWeakRef());
    }
    else {
        data->resolved = true;
        data->related = related.toWeakRef();
    }
}

template<class T>
void QpWeakRelation<T>::relate(QList<QSharedPointer<T> > related)
{
    Q_ASSERT(data->isToMany());
    if (!data->resolved)
        data->resolveFromDatabase();

    data->cleared = false;
    data->relatedList.append(Qp::Private::makeListWeak<T>(related));
}

template<class T>
void QpWeakRelation<T>::unrelate(QSharedPointer<T> related)
{
    if (!data->resolved)
        data->resolveFromDatabase();

    if (data->isToMany()) {
        int i = 0;
        Q_FOREACH(QWeakPointer<T> t, data->relatedList) {
            if (t.data() == related.data()) {
                data->relatedList.removeAt(i);
            }
            ++i;
        }
    }
    else {
        Q_ASSERT(related.data() == data->related.data());
        data->related = QWeakPointer<T>();
    }
}

template<class T>
void QpWeakRelation<T>::clear()
{
    data->cleared = true;
    data->related = QWeakPointer<T>();
    data->relatedList.clear();
}

template<class T>
class QpStrongRelationData : public QSharedData {
public:
    QString name;
    QObject *parent;
    mutable bool resolved;
    QpMetaProperty metaProperty;
    QpMetaProperty::Cardinality cardinality;
    mutable QList<QSharedPointer<T> > relatedList;
    mutable QSharedPointer<T> related;

    void resolveFromDatabase() const;
    bool isToMany() const;
};

template<class T>
bool QpStrongRelationData<T>::isToMany() const
{
    return cardinality == QpMetaProperty::OneToManyCardinality
            || cardinality == QpMetaProperty::ManyToManyCardinality;
}

template<class T>
void QpStrongRelationData<T>::resolveFromDatabase() const
{
    QList<QSharedPointer<T> > resolvedList = Qp::castList<T>(QpRelationResolver::resolveRelation(name, parent));
    resolved = true;
    relatedList.append(resolvedList);
    if (resolvedList.isEmpty())
        return;
    related = resolvedList.first();
}


template<class T>
QpStrongRelation<T>::QpStrongRelation(const QString &name, QObject *parent) :
    data(new QpStrongRelationData<T>)
{
    data->name = name;
    data->parent = parent;
    data->resolved = false;
    data->metaProperty = QpMetaObject::forClassName(parent->metaObject()->className()).metaProperty(name);
    data->cardinality = data->metaProperty.cardinality();
}

template<class T>
QpStrongRelation<T>::QpStrongRelation(const QpStrongRelation &rhs) : data(rhs.data)
{
}

template<class T>
QpStrongRelation<T> &QpStrongRelation<T>::operator=(const QpStrongRelation<T> &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

template<class T>
QpStrongRelation<T>::~QpStrongRelation()
{
}

template<class T>
QList<QSharedPointer<T> > QpStrongRelation<T>::resolveList() const
{
    if (!data->resolved)
        data->resolveFromDatabase();

    return data->relatedList;
}

template<class T>
QSharedPointer<T> QpStrongRelation<T>::resolve() const
{
    if (!data->resolved)
        data->resolveFromDatabase();

    return data->related;
}

template<class T>
bool QpStrongRelation<T>::contains(QSharedPointer<T> object) const
{
    return resolveList().contains(object);
}

template<class T>
void QpStrongRelation<T>::relate(QSharedPointer<T> related)
{
    if(!related) {
        clear();
        return;
    }

    if (data->isToMany()) {
        if (!data->resolved)
            data->resolveFromDatabase();

        data->relatedList.append(related.toWeakRef());
    }
    else {
        data->resolved = true;
        data->related = related.toWeakRef();
    }
}

template<class T>
void QpStrongRelation<T>::relate(QList<QSharedPointer<T> > related)
{
    data->relatedList.append(related);
}

template<class T>
void QpStrongRelation<T>::unrelate(QSharedPointer<T> related)
{
    data->related.clear();

    int i = 0;
    Q_FOREACH(QSharedPointer<T> t, data->relatedList) {
        if (t == related) {
            data->relatedList.removeAt(i);
        }
        ++i;
    }
}

template<class T>
void QpStrongRelation<T>::clear()
{
    if(!data->isToMany())
        data->resolved = true;

    data->related = QSharedPointer<T>();
    data->relatedList.clear();
}
