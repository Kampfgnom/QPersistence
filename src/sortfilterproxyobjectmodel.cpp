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
    if (sortRole >= Qt::UserRole + 1
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

QList<QSharedPointer<QObject> > QpSortFilterProxyObjectModelBase::objectsBase() const
{
    QList<QSharedPointer<QObject> > result;
    for (int i = 0, c = rowCount(); i < c; ++i) {
        result << objectByIndexBase(index(i, 0));
    }
    return result;
}

bool QpSortFilterProxyObjectModelBase::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid())
        return true;
    if (!right.isValid())
        return false;

    QSharedPointer<QObject> o1 = sourceQpModel()->objectByIndexBase(left);
    QSharedPointer<QObject> o2 = sourceQpModel()->objectByIndexBase(right);

    return objectLessThanBase(o1, o2);
}

bool QpSortFilterProxyObjectModelBase::objectLessThanBase(QSharedPointer<QObject> left, QSharedPointer<QObject> right) const
{
    return Qp::Private::primaryKey(left.data()) < Qp::Private::primaryKey(right.data());
}

bool QpSortFilterProxyObjectModelBase::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QSharedPointer<QObject> o = sourceQpModel()->objectByIndexBase(sourceModel()->index(source_row, 0, source_parent));

    if (!includeDeletedObjects()
        && Qp::Private::isDeleted(o.data()))
        return false;

    if (!filterAcceptsObjectBase(o))
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
