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

class QpObjectListModelBaseData;
class QpObjectListModelBase : public QAbstractListModel
{
    Q_OBJECT
public:
    QpObjectListModelBase(QpDaoBase *dao, QObject *parent);
    ~QpObjectListModelBase();

    bool canFetchMore(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void fetchMore(const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int fetchCount() const;
    void setFetchCount(int fetchCount);

    QList<QSharedPointer<QObject> > objects() const;
    void setObjects(QList<QSharedPointer<QObject> > objects);

    QSharedPointer<QObject> objectByIndex(const QModelIndex &index) const;
    QModelIndex indexForObject(QSharedPointer<QObject> object) const;

    QpDaoBase *dataAccessObject() const;

    void setCondition(const QpSqlCondition &condition);

protected slots:
    void objectInserted(QSharedPointer<QObject>);
    void objectUpdated(QSharedPointer<QObject>);
    void objectRemoved(QSharedPointer<QObject>);
    void objectMarkedAsDeleted(QSharedPointer<QObject>);

private:
    QExplicitlySharedDataPointer<QpObjectListModelBaseData> d;
};

template<class T>
class QpObjectListModel : public QpObjectListModelBase
{
public:
    explicit QpObjectListModel(QObject *parent = 0);
    explicit QpObjectListModel(QpStorage *storage, QObject *parent = 0);

    QSharedPointer<T> objectByIndex(const QModelIndex &index) const;
    QList<QSharedPointer<T> > objects() const;
    QModelIndex indexForObject(QSharedPointer<T> object) const;
};

template<class T>
QpObjectListModel<T>::QpObjectListModel(QObject *parent) :
    QpObjectListModel<T>(QpStorage::defaultStorage(), parent)
{
}

template<class T>
QpObjectListModel<T>::QpObjectListModel(QpStorage *storage, QObject *parent) :
    QpObjectListModelBase(storage->dataAccessObject<T>(), parent)
{
}

template<class T>
QSharedPointer<T> QpObjectListModel<T>::objectByIndex(const QModelIndex &index) const
{
    return qSharedPointerCast<T>(QpObjectListModelBase::objectByIndex(index));
}

template<class T>
QList<QSharedPointer<T> > QpObjectListModel<T>::objects() const
{
    return Qp::castList<T>(QpObjectListModelBase::objects());
}

template<class T>
QModelIndex QpObjectListModel<T>::indexForObject(QSharedPointer<T> object) const
{
    return QpObjectListModelBase::indexForObject(qSharedPointerCast<QObject>(object));
}

#endif // QPERSISTENCE_OBJECTLISTMODEL_H
