#include "objectlistmodel.h"

class QpObjectListModelBaseData : public QSharedData
{
public:
    QpObjectListModelBaseData() : QSharedData(),
        fetchCount(std::numeric_limits<int>::max()),
        dao(nullptr),
        objectsFromDao(true)
    {}

    int fetchCount;
    mutable QHash<QSharedPointer<QObject>, int> rows;
    QList<QSharedPointer<QObject> > objects;
    QpDaoBase *dao;
    bool objectsFromDao;
    QpSqlCondition condition;
};

QpObjectListModelBase::QpObjectListModelBase(QpDaoBase *dao, QObject *parent) :
    QAbstractListModel(parent),
    d(new QpObjectListModelBaseData)
{
    d->dao = dao;
    connect(dao, &QpDaoBase::objectCreated, this, &QpObjectListModelBase::objectInserted);
    connect(dao, &QpDaoBase::objectRemoved, this, &QpObjectListModelBase::objectRemoved);
    connect(dao, &QpDaoBase::objectUpdated, this, &QpObjectListModelBase::objectUpdated);
    connect(dao, &QpDaoBase::objectMarkedAsDeleted, this, &QpObjectListModelBase::objectMarkedAsDeleted);
}

QpObjectListModelBase::~QpObjectListModelBase()
{
}

int QpObjectListModelBase::fetchCount() const
{
    return d->fetchCount;
}

void QpObjectListModelBase::setFetchCount(int fetchCount)
{
    if(fetchCount < 0)
        d->fetchCount = std::numeric_limits<int>::max();
    else
        d->fetchCount = fetchCount;
}

QpDaoBase *QpObjectListModelBase::dataAccessObject() const
{
    return d->dao;
}

void QpObjectListModelBase::setCondition(const QpSqlCondition &condition)
{
    beginResetModel();
    d->condition = condition;
    endResetModel();
}

int QpObjectListModelBase::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int index = metaObject()->indexOfEnumerator("Columns");
    if(index != -1)
        return metaObject()->enumerator(index).keyCount();

    return d->dao->qpMetaObject().metaObject().propertyCount();
}

int QpObjectListModelBase::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return d->objects.size();
}

bool QpObjectListModelBase::canFetchMore(const QModelIndex &) const
{
    if(!d->objectsFromDao)
        return false;

    return (d->objects.size() < d->dao->count(d->condition));
}

void QpObjectListModelBase::fetchMore(const QModelIndex &/*parent*/)
{
    if(!d->objectsFromDao)
        return;

    int begin = d->objects.size();
    int remainder = d->dao->count(d->condition) - begin;
    int itemsToFetch = qMin(d->fetchCount, remainder);

    beginInsertRows(QModelIndex(), begin, begin+itemsToFetch-1);

    d->objects.append(d->dao->readAllObjects(begin, itemsToFetch, d->condition));
    for(int i = begin; i < begin + itemsToFetch; ++i) {
        d->rows.insert(d->objects.at(i), i);
    }

    endInsertRows();
}

QVariant QpObjectListModelBase::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        QSharedPointer<QObject> object = QpObjectListModelBase::objectByIndex(index);
        const QMetaObject *mo = object->metaObject();
        return mo->property(index.column()).read(object.data());
    }

    return QVariant();
}

QVariant QpObjectListModelBase::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();

    if (role == Qt::DisplayRole) {
        QpMetaObject mo = d->dao->qpMetaObject();
        return QVariant(mo.metaObject().property(section).name());
    }

    return QVariant();
}

QModelIndex QpObjectListModelBase::indexForObject(QSharedPointer<QObject> object) const
{
    if(!d->rows.contains(object))
        return QModelIndex();

    int row = d->rows.value(object);
    return index(row);
}

QSharedPointer<QObject> QpObjectListModelBase::objectByIndex(const QModelIndex &index) const
{
    Q_ASSERT(index.row() < d->objects.size());
    Q_ASSERT(index.isValid());
    return d->objects.at(index.row());
}

QList<QSharedPointer<QObject> > QpObjectListModelBase::objects() const
{
    return d->objects;
}

void QpObjectListModelBase::setObjects(QList<QSharedPointer<QObject> > objects)
{
    beginResetModel();

    if(d->dao)
        disconnect(d->dao, 0, this, 0);

    d->objectsFromDao = false;
    d->objects = objects;
    d->rows.clear();

    for(int i = 0, count = objects.size(); i < count; ++i) {
        d->rows.insert(d->objects.at(i), i);
    }

    endResetModel();
}

void QpObjectListModelBase::objectInserted(QSharedPointer<QObject> object)
{
    while(!d->rows.contains(object)) {
        if(!canFetchMore())
            return;

        fetchMore();
    }
}

void QpObjectListModelBase::objectUpdated(QSharedPointer<QObject> object)
{
    QModelIndex i;
    while(!(i = indexForObject(object)).isValid() && canFetchMore()) {
        fetchMore();
    }

    if(i.isValid())
        emit dataChanged(i, i);
}

void QpObjectListModelBase::objectRemoved(QSharedPointer<QObject> object)
{
    if(!d->rows.contains(object))
        return;

    int row = d->rows.value(object);
    beginRemoveRows(QModelIndex(), row, row);

    for(int i = row + 1; i < d->objects.size(); ++i) {
        d->rows.insert(d->objects.at(i), i - 1);
    }
    d->rows.remove(object);
    d->objects.removeAt(row);

    endRemoveRows();
}

void QpObjectListModelBase::objectMarkedAsDeleted(QSharedPointer<QObject> object)
{
    objectUpdated(object);

    if(Qp::isDeleted(object))
        objectRemoved(object);
}

