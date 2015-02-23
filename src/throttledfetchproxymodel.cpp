#include "throttledfetchproxymodel.h"

#include "sortfilterproxyobjectmodel.h"

QpThrottledFetchProxyModel::QpThrottledFetchProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
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
    if (!m_timer.hasExpired(m_throttle))
        return true;

    m_timer.start();
    return false;
}

bool QpThrottledFetchProxyModel::canFetchMore(const QModelIndex &parent) const
{
    if (isThrottled())
        return false;

    return QSortFilterProxyModel::sourceModel()->canFetchMore(QSortFilterProxyModel::mapToSource(parent));
}

