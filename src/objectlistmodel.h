#ifndef QPERSISTENCE_OBJECTLISTMODEL_H
#define QPERSISTENCE_OBJECTLISTMODEL_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QAbstractListModel>
#include <QtCore/QMetaProperty>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "dataaccessobject.h"
#include "private.h"
#include "qpersistence.h"
#include "storage.h"
#include "defaultstorage.h"

class QpObjectListModelBase : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit QpObjectListModelBase(QObject *parent = 0);

    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    virtual QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const = 0;

    int fetchCount() const;
    void setFetchCount(int fetchCount);

    virtual QModelIndex indexForObject(QSharedPointer<QObject> object) const = 0;

protected slots:
    virtual void objectInserted(QSharedPointer<QObject>) = 0;
    virtual void objectUpdated(QSharedPointer<QObject>) = 0;
    virtual void objectRemoved(QSharedPointer<QObject>) = 0;
    virtual void objectMarkedAsDeleted(QSharedPointer<QObject>) = 0;

protected:
    int m_fetchCount;
};

template<class T>
class QpObjectListModel : public QpObjectListModelBase
{
public:
    explicit QpObjectListModel(QObject *parent = 0);
    explicit QpObjectListModel(QpStorage *storage, QObject *parent = 0);

    bool canFetchMore(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void fetchMore(const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSharedPointer<T> objectByIndex(const QModelIndex &index) const;
    QList<QSharedPointer<T> > objects() const;
    QModelIndex indexForObject(QSharedPointer<T> object) const;
    QModelIndex indexForObject(QSharedPointer<QObject> object) const Q_DECL_OVERRIDE;

protected:
    void objectInserted(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
    void objectUpdated(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
    void objectRemoved(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
    void objectMarkedAsDeleted(QSharedPointer<QObject>);

private:
    mutable QHash<QSharedPointer<T>, int> m_rows;
    QList<QSharedPointer<T> > m_objects;
    QpDao<T> *m_dao;
};

template<class T>
QpObjectListModel<T>::QpObjectListModel(QObject *parent) :
    QpObjectListModel(QpStorage::defaultStorage(), parent)
{
}

template<class T>
QpObjectListModel<T>::QpObjectListModel(QpStorage *storage, QObject *parent) :
    QpObjectListModelBase(parent)
{
    m_dao = storage->dataAccessObject<T>();
    connect(m_dao, &QpDaoBase::objectCreated,
            this, &QpObjectListModel<T>::objectInserted);
    connect(m_dao, &QpDaoBase::objectRemoved,
            this, &QpObjectListModel<T>::objectRemoved);
    connect(m_dao, &QpDaoBase::objectUpdated,
            this, &QpObjectListModel<T>::objectUpdated);
    connect(m_dao, &QpDaoBase::objectMarkedAsDeleted,
            this, &QpObjectListModel<T>::objectMarkedAsDeleted);
}

template<class T>
bool QpObjectListModel<T>::canFetchMore(const QModelIndex &) const
{
    return (m_objects.size() < m_dao->count());
}

template<class T>
QModelIndex QpObjectListModel<T>::indexForObject(QSharedPointer<T> object) const
{
    if(!m_rows.contains(object))
        return QModelIndex();

    int row = m_rows.value(object);
    return index(row);
}

template<class T>
QModelIndex QpObjectListModel<T>::indexForObject(QSharedPointer<QObject> object) const
{
    return indexForObject(qSharedPointerCast<T>(object));
}

template<class T>
void QpObjectListModel<T>::fetchMore(const QModelIndex &/*parent*/)
{
    int begin = m_objects.size();
    int remainder = m_dao->count() - begin;
    int itemsToFetch = qMin(m_fetchCount, remainder);

    beginInsertRows(QModelIndex(), begin, begin+itemsToFetch-1);

    m_objects.append(m_dao->readAllObjects(begin, itemsToFetch));
    for(int i = begin; i < begin + itemsToFetch; ++i) {
        m_rows.insert(m_objects.at(i), i);
    }


    endInsertRows();
}

template<class T>
int QpObjectListModel<T>::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return T::staticMetaObject.propertyCount();
}

template<class T>
QVariant QpObjectListModel<T>::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        QSharedPointer<T> object = QpObjectListModel<T>::objectByIndex(index);
        QMetaObject mo = T::staticMetaObject;
        return mo.property(index.column()).read(object.data());
    }

    return QVariant();
}

template<class T>
QVariant QpObjectListModel<T>::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();

    if (role == Qt::DisplayRole) {
        QMetaObject mo = T::staticMetaObject;
        return QVariant(mo.property(section).name());
    }

    return QVariant();
}


template<class T>
QSharedPointer<QObject> QpObjectListModel<T>::objectByIndexBase(const QModelIndex &index) const
{
    return qSharedPointerCast<QObject>(objectByIndex(index));
}

template<class T>
QSharedPointer<T> QpObjectListModel<T>::objectByIndex(const QModelIndex &index) const
{
    Q_ASSERT(index.row() < m_objects.size());
    Q_ASSERT(index.isValid());
    return m_objects.at(index.row());
}

template<class T>
QList<QSharedPointer<T> > QpObjectListModel<T>::objects() const
{
    return m_objects;
}

template<class T>
int QpObjectListModel<T>::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_objects.size();
}

template<class T>
void QpObjectListModel<T>::objectInserted(QSharedPointer<QObject> object)
{
    QSharedPointer<T> t = qSharedPointerCast<T>(object);

    while(!m_rows.contains(t)) {
        if(!canFetchMore())
            return;

        fetchMore();
    }
}

template<class T>
void QpObjectListModel<T>::objectUpdated(QSharedPointer<QObject> object)
{
    QSharedPointer<T> t = qSharedPointerCast<T>(object);
    QModelIndex i = indexForObject(t);
    if(i.isValid())
        emit dataChanged(i, i);
}

template<class T>
void QpObjectListModel<T>::objectRemoved(QSharedPointer<QObject> object)
{
    QSharedPointer<T> t = qSharedPointerCast<T>(object);

    if(!m_rows.contains(t))
        return;

    int row = m_rows.value(t);
    beginRemoveRows(QModelIndex(), row, row);

    for(int i = row + 1; i < m_objects.size(); ++i) {
        m_rows.insert(m_objects.at(i), i - 1);
    }
    m_rows.remove(t);
    m_objects.removeAt(row);

    endRemoveRows();
}

template<class T>
void QpObjectListModel<T>::objectMarkedAsDeleted(QSharedPointer<QObject> object)
{
    objectUpdated(object);
}

#endif // QPERSISTENCE_OBJECTLISTMODEL_H
