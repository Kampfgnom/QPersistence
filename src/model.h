#ifndef QPERSISTENCE_MODEL_H
#define QPERSISTENCE_MODEL_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QAbstractProxyModel>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "private.h"

class QpModelBase
{
public:
    virtual ~QpModelBase();
    QpModelBase *sourceQpModel() const;
    virtual QModelIndex indexForObjectBase(QSharedPointer<QObject> object) const;
    virtual QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const;
    virtual QList<QSharedPointer<QObject> > objectsByIndexes(const QModelIndexList &list) const;
    virtual QList<QSharedPointer<QObject> > objectsBase() const;
    QAbstractItemModel *model() const;
    template<class T> T *findModelInHierarchy() const;
    template<class T> QList<T *> findModelsInHierarchy() const;
    template<class T> T *createProxyIfNotExistsInHierarchy();
};
template<class T>
T *QpModelBase::createProxyIfNotExistsInHierarchy()
{
    T *proxy = findModelInHierarchy<T>();
    if(proxy)
        return proxy;

    proxy = new T(model());
    proxy->setSourceModel(model());
    return proxy;
}

template<class T>
T *QpModelBase::findModelInHierarchy() const
{
    if(const T *t = dynamic_cast<const T *>(this))
        return const_cast<T *>(t);

    if(QpModelBase *source = sourceQpModel())
        return source->findModelInHierarchy<T>();

    return nullptr;
}

template<class T>
QList<T *> QpModelBase::findModelsInHierarchy() const
{
    QList<T *> result;
    if(QpModelBase *source = sourceQpModel())
        result << source->findModelsInHierarchy<T>();

    if(const T *t = dynamic_cast<const T *>(this))
        result << const_cast<T *>(t);

    return result;
}

template<class M, class T>
class QpModel
{
public:
    QSharedPointer<T> objectByIndex(const QModelIndex &index) const;
    QModelIndex indexForObject(QSharedPointer<T> object) const;
    QList<QSharedPointer<T> > objects() const;
};

template<class M, class T>
QModelIndex QpModel<M, T>::indexForObject(QSharedPointer<T> object) const
{
    return static_cast<const M *>(this)->indexForObjectBase(qSharedPointerCast<QObject>(object));
}

template<class M, class T>
QList<QSharedPointer<T> > QpModel<M, T>::objects() const
{
    return Qp::castList<T>(static_cast<const M *>(this)->objectsBase());
}

template<class M, class T>
QSharedPointer<T> QpModel<M, T>::objectByIndex(const QModelIndex &index) const
{
    return qSharedPointerCast<T>(static_cast<const M *>(this)->objectByIndexBase(index));
}

#endif // QPERSISTENCE_MODEL_H
