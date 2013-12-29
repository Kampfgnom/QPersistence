#ifndef ABSTRACTOBJECTLISTMODEL_H
#define ABSTRACTOBJECTLISTMODEL_H

#include <QAbstractListModel>

#include "dataaccessobject.h"
#include "qpersistence.h"
#include "private.h"

class QpAbstractObjectListModelBase : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit QpAbstractObjectListModelBase(QObject *parent = 0);

    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    virtual QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const = 0;

    int fetchCount() const;
    void setFetchCount(int fetchCount);

protected slots:
    virtual void objectInserted(QSharedPointer<QObject>) = 0;
    virtual void objectUpdated(QSharedPointer<QObject>) = 0;
    virtual void objectRemoved(QSharedPointer<QObject>) = 0;

protected:
    bool m_objectsFromDao;
    int m_fetchCount;
};

template<class T>
class QpAbstractObjectListModel : public QpAbstractObjectListModelBase
{
public:
    explicit QpAbstractObjectListModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int rowOf(QSharedPointer<T> object) const;

    QSharedPointer<T> objectByIndex(const QModelIndex &index) const;
    QList<QSharedPointer<T> > objects() const;
    void setObjects(const QList<QSharedPointer<T> > &objects);

    bool canFetchMore(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void fetchMore(const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
    QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const Q_DECL_OVERRIDE;


protected:
    void objectInserted(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
    void objectUpdated(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
    void objectRemoved(QSharedPointer<QObject>) Q_DECL_OVERRIDE;

private:
    mutable QHash<QSharedPointer<T>, int> m_rows;
    QList<QSharedPointer<T> > m_objects;
    void adjustExistingRows();
};

template<class T>
QpAbstractObjectListModel<T>::QpAbstractObjectListModel(QObject *parent) :
    QpAbstractObjectListModelBase(parent)
{
    QpDao<T> *dao = Qp::dataAccessObject<T>();
    connect(dao, &QpDaoBase::objectCreated,
            this, &QpAbstractObjectListModel<T>::objectInserted);
    connect(dao, &QpDaoBase::objectRemoved,
            this, &QpAbstractObjectListModel<T>::objectRemoved);
    connect(dao, &QpDaoBase::objectUpdated,
            this, &QpAbstractObjectListModel<T>::objectUpdated);
}

template<class T>
QSharedPointer<QObject> QpAbstractObjectListModel<T>::objectByIndexBase(const QModelIndex &index) const
{
    return qSharedPointerCast<QObject>(objectByIndex(index));
}

template<class T>
QSharedPointer<T> QpAbstractObjectListModel<T>::objectByIndex(const QModelIndex &index) const
{
    if(index.row() >= objects().size())
        return QSharedPointer<T>();

    return objects().at(index.row());
}

template<class T>
QList<QSharedPointer<T> > QpAbstractObjectListModel<T>::objects() const
{
    return m_objects;
}

template<class T>
void QpAbstractObjectListModel<T>::setObjects(const QList<QSharedPointer<T> > &objects)
{
    beginResetModel();
    m_objects = objects;
    m_objectsFromDao = false;
    disconnect(Qp::dataAccessObject<T>(),&QpDaoBase::objectCreated,this,0);
    endResetModel();
}

template<class T>
int QpAbstractObjectListModel<T>::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;

    return objects().size();
}

template<class T>
int QpAbstractObjectListModel<T>::rowOf(QSharedPointer<T> object) const
{
    if(!m_rows.contains(object))
        m_rows.insert(qSharedPointerCast<T>(object), objects().indexOf(object));

    return m_rows.value(qSharedPointerCast<T>(object));
}

template<class T>
void QpAbstractObjectListModel<T>::objectInserted(QSharedPointer<QObject> object)
{
    QSharedPointer<T> t = qSharedPointerCast<T>(object);
    int row = rowOf(t);
    while(row < 0 && canFetchMore()) {
        fetchMore();
        row = rowOf(t);
    }

    beginInsertRows(QModelIndex(), row, row);
    m_rows.insert(t, row);
    adjustExistingRows();
    endInsertRows();
}

template<class T>
void QpAbstractObjectListModel<T>::objectUpdated(QSharedPointer<QObject> object)
{
    int row = rowOf(qSharedPointerCast<T>(object));
    if(row < 0)
        return;

    QModelIndex i = index(row);
    emit dataChanged(i, i);
}

template<class T>
void QpAbstractObjectListModel<T>::objectRemoved(QSharedPointer<QObject> object)
{
    QSharedPointer<T> t = qSharedPointerCast<T>(object);
    int row = rowOf(t);
    if(row >= 0)
        beginRemoveRows(QModelIndex(), row, row);

    m_rows.remove(t);
    m_objects.removeAll(t);
    adjustExistingRows();

    if(row >= 0)
        endRemoveRows();
}

template<class T>
bool QpAbstractObjectListModel<T>::canFetchMore(const QModelIndex &/*parent*/) const
{
    if(!m_objectsFromDao)
        return false;

    return (m_objects.size() < Qp::count<T>());
}

template<class T>
void QpAbstractObjectListModel<T>::fetchMore(const QModelIndex &/*parent*/)
{
    QpDao<T> *dao = Qp::dataAccessObject<T>();
    int remainder = dao->count() - m_objects.size();
    int itemsToFetch = qMin(m_fetchCount, remainder);

    beginInsertRows(QModelIndex(), m_objects.size(), m_objects.size()+itemsToFetch-1);

    m_objects.append(dao->readAllObjects(m_objects.size(), itemsToFetch));

    endInsertRows();
}

template<class T>
void QpAbstractObjectListModel<T>::adjustExistingRows()
{
    int i = 0;
    foreach(QSharedPointer<T> object, objects()) {
        if(m_rows.contains(object))
            m_rows.insert(object, i);
        ++i;
    }
}

#endif // ABSTRACTOBJECTLISTMODEL_H
