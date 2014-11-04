#ifndef QPERSISTENCE_SORTFILTERPROXYOBJECTMODEL_H
#define QPERSISTENCE_SORTFILTERPROXYOBJECTMODEL_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSortFilterProxyModel>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "model.h"
#include "objectlistmodel.h"
#include "throttledfetchproxymodel.h"

class QpSortFilterProxyObjectModelBase : public QSortFilterProxyModel, public QpModelBase
{
    Q_OBJECT
public:
    explicit QpSortFilterProxyObjectModelBase(QObject *parent = 0);

    int sortRoleCount() const;
    QString sortRoleTitle(int sortRole) const;
    virtual QStringList sortRoles() const;

    bool includeDeletedObjects() const;
    void setIncludeDeletedObjects(bool includeDeletedObjects);

    QList<QSharedPointer<QObject> > objectsBase() const Q_DECL_OVERRIDE;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;
    bool lessThan(QSharedPointer<QObject> left, QSharedPointer<QObject> right) const;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const Q_DECL_OVERRIDE;
    virtual bool filterAcceptsObjectBase(QSharedPointer<QObject> object) const = 0;

private:
    bool m_includeDeletedObjects;
};

template<class T>
class QpSortFilterProxyObjectModel : public QpSortFilterProxyObjectModelBase, public QpModel<QpSortFilterProxyObjectModel<T>, T>
{
public:
    explicit QpSortFilterProxyObjectModel(QObject *parent = 0);
    QpSortFilterProxyObjectModel(QpObjectListModelBase *sourceModel, QObject *parent = 0);

    QpObjectListModel<T> *sourceModel() const;

protected:
    bool filterAcceptsObjectBase(QSharedPointer<QObject> object) const;
    virtual bool filterAcceptsObject(QSharedPointer<T>) const;
};

template<class T>
QpSortFilterProxyObjectModel<T>::QpSortFilterProxyObjectModel(QObject *parent) :
    QpSortFilterProxyObjectModelBase(parent)
{
}

template<class T>
QpSortFilterProxyObjectModel<T>::QpSortFilterProxyObjectModel(QpObjectListModelBase *sourceModel, QObject *parent) :
    QpSortFilterProxyObjectModelBase(parent)
{
    QAbstractProxyModel::setSourceModel(sourceModel);
}

template<class T>
QpObjectListModel<T> *QpSortFilterProxyObjectModel<T>::sourceModel() const
{
    return static_cast<QpObjectListModel<T> *>(QAbstractProxyModel::sourceModel());
}

template<class T>
bool QpSortFilterProxyObjectModel<T>::filterAcceptsObjectBase(QSharedPointer<QObject> object) const
{
    return filterAcceptsObject(qSharedPointerCast<T>(object));
}

template<class T>
bool QpSortFilterProxyObjectModel<T>::filterAcceptsObject(QSharedPointer<T>) const
{
    return true;
}

#endif // QPERSISTENCE_SORTFILTERPROXYOBJECTMODEL_H
