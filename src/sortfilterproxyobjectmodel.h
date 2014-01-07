#ifndef SORTFILTERPROXYOBJECTMODEL_H
#define SORTFILTERPROXYOBJECTMODEL_H

#include <QSortFilterProxyModel>

#include "objectlistmodel.h"

class QpSortFilterProxyObjectModelBase : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit QpSortFilterProxyObjectModelBase(QObject *parent = 0);

    int sortRoleCount() const;
    QString sortRoleTitle(int sortRole) const;
    virtual QStringList sortRoles() const;
};

template<class T>
class QpSortFilterProxyObjectModel : public QpSortFilterProxyObjectModelBase
{
public:
    explicit QpSortFilterProxyObjectModel(QpObjectListModel<T> *sourceModel, QObject *parent = 0);

    QpObjectListModel<T> *sourceModel() const;
    QSharedPointer<T> objectByIndex(const QModelIndex &index) const;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    virtual bool lessThan(QSharedPointer<T> left, QSharedPointer<T> right) const;
};


template<class T>
QpSortFilterProxyObjectModel<T>::QpSortFilterProxyObjectModel(QpObjectListModel<T> *sourceModel, QObject *parent) :
    QpSortFilterProxyObjectModelBase(parent)
{
    setSourceModel(sourceModel);
}

template<class T>
QpObjectListModel<T> *QpSortFilterProxyObjectModel<T>::sourceModel() const
{
    return static_cast<QpObjectListModel<T> *>(QSortFilterProxyModel::sourceModel());

}

template<class T>
QSharedPointer<T> QpSortFilterProxyObjectModel<T>::objectByIndex(const QModelIndex &index) const
{
    QModelIndex i = mapToSource(index);
    return sourceModel()->objectByIndex(i);
}


template<class T>
bool QpSortFilterProxyObjectModel<T>::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QSharedPointer<T> o1 = sourceModel()->objectByIndex(left);
    QSharedPointer<T> o2 = sourceModel()->objectByIndex(right);

    return lessThan(o1, o2);
}

template<class T>
bool QpSortFilterProxyObjectModel<T>::lessThan(QSharedPointer<T> left, QSharedPointer<T> right) const
{
    return left < right;
}

#endif // SORTFILTERPROXYOBJECTMODEL_H
