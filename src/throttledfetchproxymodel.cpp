#include "throttledfetchproxymodel.h"

#include "sortfilterproxyobjectmodel.h"

QpThrottledFetchProxyModel::QpThrottledFetchProxyModel(QObject *parent) :
    QIdentityProxyModel(parent),
    m_throttle(1000)
{
}

int QpThrottledFetchProxyModel::throttle() const
{
    return m_throttle;
}

void QpThrottledFetchProxyModel::setThrottle(int throttle)
{
    m_throttle = throttle;
}

bool QpThrottledFetchProxyModel::isThrottled() const
{
    if(!m_timer.hasExpired(m_throttle))
        return true;

    m_timer.start();
    return false;
}

bool QpThrottledFetchProxyModel::canFetchMore(const QModelIndex &parent) const
{
    if(isThrottled())
        return false;

    return sourceModel()->canFetchMore(mapToSource(parent));
}

QModelIndex QpThrottledFetchProxyModel::indexForObject(QSharedPointer<QObject> object) const
{
    QAbstractItemModel *source = QIdentityProxyModel::sourceModel();
    if(QpSortFilterProxyObjectModelBase *model = qobject_cast<QpSortFilterProxyObjectModelBase *>(source)) {
        return mapFromSource(model->indexForObject(object));
    }
    else if(QpObjectListModelBase *model = qobject_cast<QpObjectListModelBase *>(source)) {
        return mapFromSource(model->indexForObject(object));
    }
    else if(QpThrottledFetchProxyModel *model = qobject_cast<QpThrottledFetchProxyModel *>(source)) {
        return mapFromSource(model->indexForObject(object));
    }

    return QModelIndex();
}
