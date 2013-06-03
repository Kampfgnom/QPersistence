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

    virtual QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const = 0;

protected slots:
    virtual void objectInserted(QSharedPointer<QObject>) = 0;
    virtual void objectUpdated(QSharedPointer<QObject>) = 0;
    virtual void objectRemoved(QSharedPointer<QObject>) = 0;
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

    bool canFetchMore(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    void fetchMore(const QModelIndex &parent) Q_DECL_OVERRIDE;
    QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const Q_DECL_OVERRIDE;

protected:
    void objectInserted(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
    void objectUpdated(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
    void objectRemoved(QSharedPointer<QObject>) Q_DECL_OVERRIDE;
private:
    mutable QHash<QSharedPointer<T>, int> m_rows;
    QList<QSharedPointer<T> > m_objects;
    QpDao<T> *m_dao;
    int m_fetchCount;
    void adjustExistingRows();
};

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
QpAbstractObjectListModel<T>::QpAbstractObjectListModel(QObject *parent) :
    QpAbstractObjectListModelBase(parent)
{
    m_fetchCount = 50;
    m_dao = Qp::dataAccessObject<T>();
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
    int row = rowOf(qSharedPointerCast<T>(object));
    if(row < 0)
        return;

    beginInsertRows(QModelIndex(), row, row);
    m_rows.insert(qSharedPointerCast<T>(object), row);
    adjustExistingRows();
    endInsertRows();
}

template<class T>
void QpAbstractObjectListModel<T>::objectUpdated(QSharedPointer<QObject> object)
{
    int row = rowOf(qSharedPointerCast<T>(object));
    if(row < 0)
        return;

    emit dataChanged(index(row), index(row));
}

template<class T>
void QpAbstractObjectListModel<T>::objectRemoved(QSharedPointer<QObject> object)
{
    int row = rowOf(qSharedPointerCast<T>(object));
    if(row < 0)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_rows.remove(qSharedPointerCast<T>(object));
    m_objects.removeAll(qSharedPointerCast<T>(object));
    adjustExistingRows();
    endRemoveRows();
}

template<class T>
bool QpAbstractObjectListModel<T>::canFetchMore(const QModelIndex &/*parent*/) const
{
    if (m_objects.size() < m_dao->count())
        return true;
    else
        return false;
}

template<class T>
void QpAbstractObjectListModel<T>::fetchMore(const QModelIndex &/*parent*/)
{
    int remainder = m_dao->count() - m_objects.size();
    int itemsToFetch = qMin(m_fetchCount, remainder);

    beginInsertRows(QModelIndex(), m_objects.size(), m_objects.size()+itemsToFetch-1);

    m_objects.append(Qp::Private::castList<T>(m_dao->readAllObjects(m_objects.size(), itemsToFetch)));

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
