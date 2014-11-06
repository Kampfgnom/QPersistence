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

    if(!proxyThis)
        return nullptr;

    QpModelBase *source = dynamic_cast<QpModelBase *>(proxyThis->sourceModel());
    Q_ASSERT_X(source, Q_FUNC_INFO, "The source of a QpModelBase has to be a QpModelBase");
    return source;
}

QModelIndex QpModelBase::indexForObjectBase(QSharedPointer<QObject> object) const
{
    const QAbstractProxyModel *proxyThis = dynamic_cast<const QAbstractProxyModel *>(this);
    Q_ASSERT_X(proxyThis, Q_FUNC_INFO, "Every QpModelBase must inherit QAbstractProxyModel, or implement indexForObjectBase");
    return proxyThis->mapFromSource(sourceQpModel()->indexForObjectBase(object));
}

QSharedPointer<QObject> QpModelBase::objectByIndexBase(const QModelIndex &index) const
{
    const QAbstractProxyModel *proxyThis = dynamic_cast<const QAbstractProxyModel *>(this);
    Q_ASSERT_X(proxyThis, Q_FUNC_INFO, "Every QpModelBase must inherit QAbstractProxyModel, or implement objectByIndexBase");
    return sourceQpModel()->objectByIndexBase(proxyThis->mapToSource(index));
}

QList<QSharedPointer<QObject> > QpModelBase::objectsBase() const
{
    QpModelBase *source = sourceQpModel();
    Q_ASSERT_X(source, Q_FUNC_INFO, "Every QpModelBase must inherit QAbstractProxyModel, or implement objectsBase");
    return source->objectsBase();
}
