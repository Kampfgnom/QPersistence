#ifndef QPERSISTENCE_OBJECTLISTMODEL_H
#define QPERSISTENCE_OBJECTLISTMODEL_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QAbstractListModel>
#include <QtCore/QMetaProperty>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "dataaccessobject.h"
#include "defaultstorage.h"
#include "model.h"
#include "private.h"
#include "qpersistence.h"
#include "storage.h"

class QpObjectListModelBaseData;
class QpObjectListModelBase : public QAbstractListModel, public QpModelBase
{
    Q_OBJECT
public:
    QpObjectListModelBase(QpDataAccessObjectBase *dao, QObject *parent);
    ~QpObjectListModelBase();

    bool canFetchMore(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void fetchMore(const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int fetchCount() const;
    void setFetchCount(int fetchCount);

    void setObjects(QList<QSharedPointer<QObject> > objectsBase);

    QList<QSharedPointer<QObject> > objectsBase() const Q_DECL_OVERRIDE;
    QSharedPointer<QObject> objectByIndexBase(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QModelIndex indexForObjectBase(QSharedPointer<QObject> object) const Q_DECL_OVERRIDE;

    QpDataAccessObjectBase *dataAccessObject() const;

    void setCondition(const QpCondition &condition);

protected slots:
    void objectInserted(QSharedPointer<QObject>);
    void objectUpdated(QSharedPointer<QObject>);
    void objectRemoved(QSharedPointer<QObject>);
    void objectMarkedAsDeleted(QSharedPointer<QObject>);
    void objectUndeleted(QSharedPointer<QObject>);

private:
    QExplicitlySharedDataPointer<QpObjectListModelBaseData> d;
};

template<class T>
class QpObjectListModel : public QpObjectListModelBase, public QpModel<QpObjectListModel<T>, T>
{
public:
    explicit QpObjectListModel(QObject *parent = 0);
    explicit QpObjectListModel(QpStorage *storage, QObject *parent = 0);
};

template<class T>
QpObjectListModel<T>::QpObjectListModel(QObject *parent) :
    QpObjectListModel<T>(Qp::defaultStorage(), parent)
{
}

template<class T>
QpObjectListModel<T>::QpObjectListModel(QpStorage *storage, QObject *parent) :
    QpObjectListModelBase(storage->dataAccessObject<T>(), parent)
{
}

#endif // QPERSISTENCE_OBJECTLISTMODEL_H
