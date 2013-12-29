#include "abstractobjectlistmodel.h"

QpAbstractObjectListModelBase::QpAbstractObjectListModelBase(QObject *parent) :
    QAbstractListModel(parent),
    m_objectsFromDao(true),
    m_fetchCount(std::numeric_limits<int>::max())
{
}

int QpAbstractObjectListModelBase::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int index = metaObject()->indexOfEnumerator("Columns");
    Q_ASSERT_X(index != -1,
               Q_FUNC_INFO,
               "You must either specify an enum named \"Columns\" in you ObjectList model, or override columnCount");
    return metaObject()->enumerator(index).keyCount();
}


int QpAbstractObjectListModelBase::fetchCount() const
{
    return m_fetchCount;
}

void QpAbstractObjectListModelBase::setFetchCount(int fetchCount)
{
    m_fetchCount = fetchCount;
}
