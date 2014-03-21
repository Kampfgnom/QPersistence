#include "objectlistmodel.h"

QpObjectListModelBase::QpObjectListModelBase(QObject *parent) :
    QAbstractListModel(parent),
    m_fetchCount(std::numeric_limits<int>::max()),
    m_objectsFromDao(true)
{
}

int QpObjectListModelBase::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int index = metaObject()->indexOfEnumerator("Columns");
    Q_ASSERT_X(index != -1,
               Q_FUNC_INFO,
               "You must either specify an enum named \"Columns\" in you ObjectList model, or override columnCount");
    return metaObject()->enumerator(index).keyCount();
}


int QpObjectListModelBase::fetchCount() const
{
    return m_fetchCount;
}

void QpObjectListModelBase::setFetchCount(int fetchCount)
{
    m_fetchCount = fetchCount;
}



