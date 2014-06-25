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

