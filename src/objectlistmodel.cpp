#include "objectlistmodel.h"


/******************************************************************************
 * QpObjectListModelBaseData
 */
class QpObjectListModelBaseData : public QSharedData
{
public:
    QpObjectListModelBaseData() :
        QSharedData(),
        fetchCount(std::numeric_limits<int>::max()),
        dao(nullptr),
        objectsFromDao(true)
    {
    }

    int fetchCount;
    mutable QHash<QSharedPointer<QObject>, int> rows;
    QList<QSharedPointer<QObject> > objects;
    QpDataAccessObjectBase *dao;
    bool objectsFromDao;
    QpCondition condition;
};


/******************************************************************************
 * QpObjectListModelBase
 */
QpObjectListModelBase::QpObjectListModelBase(QpDataAccessObjectBase *dao, QObject *parent) :
    QAbstractListModel(parent),
    d(new QpObjectListModelBaseData)
{
    d->dao = dao;
    connect(dao, &QpDataAccessObjectBase::objectCreated, this, &QpObjectListModelBase::objectInserted);
    connect(dao, &QpDataAccessObjectBase::objectRemoved, this, &QpObjectListModelBase::objectRemoved);
    connect(dao, &QpDataAccessObjectBase::objectUpdated, this, &QpObjectListModelBase::objectUpdated);
    connect(dao, &QpDataAccessObjectBase::objectMarkedAsDeleted, this, &QpObjectListModelBase::objectMarkedAsDeleted);
    connect(dao, &QpDataAccessObjectBase::objectUndeleted, this, &QpObjectListModelBase::objectUndeleted);
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
    if (fetchCount < 0)
        d->fetchCount = std::numeric_limits<int>::max();
    else
        d->fetchCount = fetchCount;
}

QpDataAccessObjectBase *QpObjectListModelBase::dataAccessObject() const
{
    return d->dao;
}

void QpObjectListModelBase::setCondition(const QpCondition &condition)
{
    beginResetModel();
    d->condition = condition;
    endResetModel();
}

int QpObjectListModelBase::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int index = metaObject()->indexOfEnumerator("Columns");
    if (index != -1)
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
    if (!d->objectsFromDao)
        return false;

    return (d->objects.size() < d->dao->count(d->condition));
}

void QpObjectListModelBase::fetchMore(const QModelIndex &parent)
{
    Q_UNUSED(parent)

    if (!d->objectsFromDao)
        return;

    int begin = d->objects.size();
    int remainder = d->dao->count(d->condition) - begin;
    int itemsToFetch = qMin(d->fetchCount, remainder);

    beginInsertRows(QModelIndex(), begin, begin+itemsToFetch-1);

    d->objects.append(d->dao->readAllObjects(begin, itemsToFetch, d->condition));
    for (int i = begin; i < begin + itemsToFetch; ++i) {
        d->rows.insert(d->objects.at(i), i);
    }

    endInsertRows();
}

QVariant QpObjectListModelBase::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        QSharedPointer<QObject> object = QpObjectListModelBase::objectByIndexBase(index);
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

QModelIndex QpObjectListModelBase::indexForObjectBase(QSharedPointer<QObject> object) const
{
    while (!d->rows.contains(object)) {
        if (!canFetchMore())
            return QModelIndex();

        const_cast<QpObjectListModelBase *>(this)->fetchMore();
    }

    int row = d->rows.value(object);
    return index(row);
}

QSharedPointer<QObject> QpObjectListModelBase::objectByIndexBase(const QModelIndex &index) const
{
    Q_ASSERT(index.row() < d->objects.size());
    Q_ASSERT(index.isValid());
    return d->objects.at(index.row());
}

QList<QSharedPointer<QObject> > QpObjectListModelBase::objectsBase() const
{
    return d->objects;
}

void QpObjectListModelBase::setObjects(QList<QSharedPointer<QObject> > objects)
{
    beginResetModel();

    if (d->dao)
        disconnect(d->dao, 0, this, 0);

    d->objectsFromDao = false;
    d->objects = objects;
    d->rows.clear();

    for (int i = 0, count = objects.size(); i < count; ++i) {
        d->rows.insert(d->objects.at(i), i);
    }

    endResetModel();
}

void QpObjectListModelBase::objectInserted(QSharedPointer<QObject> object)
{
    // fetches more items until the object is fetched
    indexForObjectBase(object);
}

void QpObjectListModelBase::objectUpdated(QSharedPointer<QObject> object)
{
    // fetches more items until the object is fetched
    QModelIndex i = indexForObjectBase(object);

    if (i.isValid())
        emit dataChanged(i, index(i.row(), columnCount(QModelIndex()) - 1));
}

void QpObjectListModelBase::objectRemoved(QSharedPointer<QObject> object)
{
    if (!d->rows.contains(object))
        return;

    int row = d->rows.value(object);
    beginRemoveRows(QModelIndex(), row, row);

    for (int i = row + 1; i < d->objects.size(); ++i) {
        d->rows.insert(d->objects.at(i), i - 1);
    }
    d->rows.remove(object);
    d->objects.removeAt(row);

    endRemoveRows();
}

void QpObjectListModelBase::objectMarkedAsDeleted(QSharedPointer<QObject> object)
{
    objectUpdated(object);

    if (Qp::Private::isDeleted(object.data()))
        objectRemoved(object);
}

void QpObjectListModelBase::objectUndeleted(QSharedPointer<QObject> object)
{
    if (!d->objectsFromDao || d->rows.contains(object))
        return;

    int index = d->dao->count(d->condition
                              && QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY, QpCondition::LessThan, Qp::Private::primaryKey(object.data())));

    beginInsertRows(QModelIndex(), index, index);

    d->objects.insert(index, object);
    for (int i = index, c = d->objects.count(); i < c; ++i) {
        d->rows.insert(d->objects.at(i), i);
    }

    endInsertRows();
}

