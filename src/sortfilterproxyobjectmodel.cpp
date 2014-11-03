#include "sortfilterproxyobjectmodel.h"

#include "throttledfetchproxymodel.h"

QpSortFilterProxyObjectModelBase::QpSortFilterProxyObjectModelBase(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_includeDeletedObjects(false)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortLocaleAware(true);
}

int QpSortFilterProxyObjectModelBase::sortRoleCount() const
{
    return sortRoles().size();
}

QString QpSortFilterProxyObjectModelBase::sortRoleTitle(int sortRole) const
{
    if(sortRole >= Qt::UserRole + 1
       && sortRole > sortRoleCount())
        sortRole -= Qt::UserRole + 1;

    return sortRoles().at(sortRole);
}

QStringList QpSortFilterProxyObjectModelBase::sortRoles() const
{
    return QStringList();
}

bool QpSortFilterProxyObjectModelBase::includeDeletedObjects() const
{
    return m_includeDeletedObjects;
}

void QpSortFilterProxyObjectModelBase::setIncludeDeletedObjects(bool includeDeletedObjects)
{
    m_includeDeletedObjects = includeDeletedObjects;
}

QModelIndex QpSortFilterProxyObjectModelBase::indexForObject(QSharedPointer<QObject> object) const
{
    QAbstractItemModel *source = QSortFilterProxyModel::sourceModel();
    if(QpSortFilterProxyObjectModelBase *model = qobject_cast<QpSortFilterProxyObjectModelBase *>(source)) {
        return mapFromSource(model->indexForObject(object));
    }
    else if(QpObjectListModelBase *model2 = qobject_cast<QpObjectListModelBase *>(source)) {
        return mapFromSource(model2->indexForObject(object));
    }
    else if(QpThrottledFetchProxyModel *model3 = qobject_cast<QpThrottledFetchProxyModel *>(source)) {
        return mapFromSource(model3->indexForObject(object));
    }

    return QModelIndex();
}

QSharedPointer<QObject> QpSortFilterProxyObjectModelBase::objectByIndex(const QModelIndex &index) const
{
    QModelIndex i = mapToSource(index);
    QAbstractItemModel *source = QSortFilterProxyModel::sourceModel();
    if(QpSortFilterProxyObjectModelBase *model = qobject_cast<QpSortFilterProxyObjectModelBase *>(source)) {
        return model->objectByIndex(i);
    }
    else if(QpObjectListModelBase *model2 = qobject_cast<QpObjectListModelBase *>(source)) {
        return model2->objectByIndex(i);
    }
    else if(QpThrottledFetchProxyModel *model3 = qobject_cast<QpThrottledFetchProxyModel *>(source)) {
        return model3->objectByIndex(i);
    }

    return QSharedPointer<QObject>();
}

QList<QSharedPointer<QObject> > QpSortFilterProxyObjectModelBase::objects() const
{
    QList<QSharedPointer<QObject> > result;
    for(int i = 0, c = rowCount(); i < c; ++i) {
        result << objectByIndex(index(i, 0));
    }
    return result;
}

QpObjectListModelBase *QpSortFilterProxyObjectModelBase::sourceModel() const
{
    QAbstractItemModel *source = QSortFilterProxyModel::sourceModel();
    while(QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(source))
        source = proxy->sourceModel();

    return static_cast<QpObjectListModelBase *>(source);
}

bool QpSortFilterProxyObjectModelBase::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if(!left.isValid())
        return true;
    if(!right.isValid())
        return false;

    QSharedPointer<QObject> o1 = sourceModel()->objectByIndex(left);
    QSharedPointer<QObject> o2 = sourceModel()->objectByIndex(right);

    return lessThan(o1, o2);
}

bool QpSortFilterProxyObjectModelBase::lessThan(QSharedPointer<QObject> left, QSharedPointer<QObject> right) const
{
    return Qp::Private::primaryKey(left.data()) < Qp::Private::primaryKey(right.data());
}

bool QpSortFilterProxyObjectModelBase::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QSharedPointer<QObject> o = sourceModel()->objectByIndex(sourceModel()->index(source_row, 0, source_parent));

    if(!includeDeletedObjects()
       && Qp::Private::isDeleted(o.data()))
        return false;

    if(!filterAcceptsObjectBase(o))
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
