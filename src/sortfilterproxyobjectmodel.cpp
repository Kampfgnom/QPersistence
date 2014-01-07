#include "sortfilterproxyobjectmodel.h"

QpSortFilterProxyObjectModelBase::QpSortFilterProxyObjectModelBase(QObject *parent) :
    QSortFilterProxyModel(parent)
{

}

int QpSortFilterProxyObjectModelBase::sortRoleCount() const
{
    return sortRoles().size();
}

QString QpSortFilterProxyObjectModelBase::sortRoleTitle(int sortRole) const
{
    return sortRoles().at(sortRole);
}

QStringList QpSortFilterProxyObjectModelBase::sortRoles() const
{
    return QStringList();
}
