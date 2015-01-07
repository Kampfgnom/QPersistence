#include "model.h"

#include "sortfilterproxyobjectmodel.h"
#include "objectlistmodel.h"
#include "throttledfetchproxymodel.h"

QpModelBase::~QpModelBase()
{
}

QpModelBase *QpModelBase::sourceQpModel() const
{
    const QAbstractProxyModel *proxyThis = dynamic_cast<const QAbstractProxyModel *>(this);

    if (!proxyThis)
        return nullptr;

    if (!proxyThis->sourceModel())
        return nullptr;

    QpModelBase *source = dynamic_cast<QpModelBase *>(proxyThis->sourceModel());
    Q_ASSERT_X(source, Q_FUNC_INFO, "The source of a QpModelBase has to be a QpModelBase");
    return source;
}

QModelIndex QpModelBase::indexForObjectBase(QSharedPointer<QObject> object) const
{
    if (!object) {
        return QModelIndex();
    }
    const QAbstractProxyModel *proxyThis = dynamic_cast<const QAbstractProxyModel *>(this);
    Q_ASSERT_X(proxyThis, Q_FUNC_INFO, "Every QpModelBase must inherit QAbstractProxyModel, or implement indexForObjectBase");
    return proxyThis->mapFromSource(sourceQpModel()->indexForObjectBase(object));
}

QSharedPointer<QObject> QpModelBase::objectByIndexBase(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QSharedPointer<QObject>();
    }
    const QAbstractProxyModel *proxyThis = dynamic_cast<const QAbstractProxyModel *>(this);
    Q_ASSERT_X(proxyThis, Q_FUNC_INFO, "Every QpModelBase must inherit QAbstractProxyModel, or implement objectByIndexBase");
    return sourceQpModel()->objectByIndexBase(proxyThis->mapToSource(index));
}

QList<QSharedPointer<QObject> > QpModelBase::objectsByIndexes(const QModelIndexList &list) const
{
    QList<QSharedPointer<QObject> > result;
    foreach (QModelIndex index, list) {
        if (index.column() != 0)
            continue;

        result << objectByIndexBase(index);
    }
    return result;
}

QList<QSharedPointer<QObject> > QpModelBase::objectsBase() const
{
    QpModelBase *source = sourceQpModel();
    Q_ASSERT_X(source, Q_FUNC_INFO, "Every QpModelBase must inherit QAbstractProxyModel, or implement objectsBase");
    return source->objectsBase();
}

QAbstractItemModel *QpModelBase::model() const
{
    return const_cast<QAbstractItemModel *>(dynamic_cast<const QAbstractItemModel *>(this));
}
