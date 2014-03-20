#ifndef QPERSISTENCE_SORTFILTERPROXYOBJECTMODEL_H
#define QPERSISTENCE_SORTFILTERPROXYOBJECTMODEL_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSortFilterProxyModel>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "objectlistmodel.h"

class QpSortFilterProxyObjectModelBase : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit QpSortFilterProxyObjectModelBase(QObject *parent = 0);

    int sortRoleCount() const;
    QString sortRoleTitle(int sortRole) const;
    virtual QStringList sortRoles() const;

    bool includeDeletedObjects() const;
    void setIncludeDeletedObjects(bool includeDeletedObjects);

private:
    bool m_includeDeletedObjects;
};

template<class T>
class QpSortFilterProxyObjectModel : public QpSortFilterProxyObjectModelBase
{
public:
    explicit QpSortFilterProxyObjectModel(QpObjectListModel<T> *sourceModel, QObject *parent = 0);

    QpObjectListModel<T> *sourceModel() const;
    QSharedPointer<T> objectByIndex(const QModelIndex &index) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;
    virtual bool lessThan(QSharedPointer<T> left, QSharedPointer<T> right) const;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const Q_DECL_OVERRIDE;
    virtual bool filterAcceptsObject(QSharedPointer<T> object) const;
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
QModelIndex QpSortFilterProxyObjectModel<T>::index(int row, int column, const QModelIndex &parent) const
{
    return QSortFilterProxyModel::index(row, column, parent);
}


template<class T>
bool QpSortFilterProxyObjectModel<T>::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(sortRole() <= Qt::UserRole)
        return QSortFilterProxyModel::lessThan(left, right);

    QSharedPointer<T> o1 = sourceModel()->objectByIndex(left);
    QSharedPointer<T> o2 = sourceModel()->objectByIndex(right);

    return lessThan(o1, o2);
}

template<class T>
bool QpSortFilterProxyObjectModel<T>::lessThan(QSharedPointer<T> left, QSharedPointer<T> right) const
{
    return left < right;
}

template<class T>
bool QpSortFilterProxyObjectModel<T>::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QSharedPointer<T> o = sourceModel()->objectByIndex(sourceModel()->index(source_row, 0, source_parent));

    if(!includeDeletedObjects()
       && Qp::isDeleted(o))
        return false;

    if(filterRole() < Qt::UserRole)
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);

    return filterAcceptsObject(o);
}

template<class T>
bool QpSortFilterProxyObjectModel<T>::filterAcceptsObject(QSharedPointer<T>) const
{
    return true;
}

#endif // QPERSISTENCE_SORTFILTERPROXYOBJECTMODEL_H
