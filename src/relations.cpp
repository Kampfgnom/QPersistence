#include "relations.h"
#include <QSharedData>

#include "relationresolver.h"
#include "qpersistence.h"

template<class T>
QpWeakRelation<T>::QpWeakRelation(const QString &name, QObject *parent) : data(new QpWeakRelationData)
{
    data->name = name;
    data->parent = parent;
    data->resolved = false;
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
    if(!data->resolved || !checkPointers())
        return resolveFromDatabase();

    return Qp::makeListStrong<T>(data->relatedList);
}

template<class T>
QSharedPointer<T> QpWeakRelation<T>::resolve() const
{
    QSharedPointer<T> strong = data->related.toStrongRef();
    if(!data->resolved || !strong) {
        QList<QSharedPointer<T> > list = resolveFromDatabase();
        if(list.isEmpty())
            return QSharedPointer<T>();

        Q_ASSERT(list.size() == 1);
        strong = list.first();
    }

    return strong;
}

template<class T>
void QpWeakRelation<T>::relate(QSharedPointer<T> related)
{
    if(!data->resolved)
        resolveFromDatabase();

    data->relatedList.append(related.toWeakRef());
    data->related = related.toWeakRef();
}

template<class T>
void QpWeakRelation<T>::relate(QList<QSharedPointer<T> > related)
{
    if(!data->resolved)
        resolveFromDatabase();

    data->relatedList.append(Qp::makeListWeak<T>(related));
}

template<class T>
void QpWeakRelation<T>::unrelate(QSharedPointer<T> related)
{
    if(!data->resolved)
        resolveFromDatabase();

    Q_ASSERT(related.data() == data->related.data());
    data->related.clear();

    int i = 0;
    Q_FOREACH(QWeakPointer<T> t, data->relatedList) {
        if(t.data() == related.data()) {
            data->relatedList.removeAt(i);
        }
        ++i;
    }
}

template<class T>
void QpWeakRelation<T>::clear()
{
    data->related = QWeakPointer<T>();
    data->relatedList.clear();
}

template<class T>
bool QpWeakRelation<T>::checkPointers() const
{
    Q_FOREACH(QWeakPointer<T> t, data->relatedList) {
        if(!t.toStrongRef())
            return false;
    }
    return true;
}

template<class T>
QList<QSharedPointer<T> > QpWeakRelation<T>::resolveFromDatabase() const
{
    QpRelationResolver resolver;
    QList<QSharedPointer<T> > resolved = Qp::Private::castList<T>(resolver.resolveRelation(data->name, data->parent));
    data->resolved = true;
    data->relatedList = Qp::makeListWeak<T>(resolved);
    if(resolved.isEmpty())
        return QList<QSharedPointer<T> >();
    data->related = resolved.first();
    return resolved;
}





template<class T>
QpStrongRelation<T>::QpStrongRelation(const QString &name, QObject *parent) : data(new QpStrongRelationData)
{
    data->name = name;
    data->parent = parent;
    data->resolved = false;
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
    if(!data->resolved)
        resolveFromDatabase();

    return data->relatedList;
}

template<class T>
QSharedPointer<T> QpStrongRelation<T>::resolve() const
{
    if(!data->resolved || !data->related)
        resolveFromDatabase();

    return data->related;
}

template<class T>
void QpStrongRelation<T>::relate(QSharedPointer<T> related)
{
    data->relatedList.append(related);
    data->related = related;
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
        if(t == related) {
            data->relatedList.removeAt(i);
        }
        ++i;
    }
}

template<class T>
void QpStrongRelation<T>::clear()
{
    data->related = QWeakPointer<T>();
    data->relatedList.clear();
}

template<class T>
void QpStrongRelation<T>::resolveFromDatabase() const
{
    QpRelationResolver resolver;
    QList<QSharedPointer<T> > resolved = Qp::Private::castList<T>(resolver.resolveRelation(data->name, data->parent));
    data->resolved = true;
    data->relatedList = resolved;
    if(resolved.isEmpty())
        return;
    data->related = resolved.first();
}
