#include "objectlistmodel.h"

template<class T>
QpObjectListModel<T>::QpObjectListModel(QObject *parent) :
    QpAbstractObjectListModel<T>(parent)
{
}

template<class T>
int QpObjectListModel<T>::columnCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;

    return T::staticMetaObject.propertyCount();
}

template<class T>
QVariant QpObjectListModel<T>::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role == Qt::DisplayRole) {
        QSharedPointer<T> object = QpAbstractObjectListModel<T>::objectByIndex(index);
        QMetaObject mo = T::staticMetaObject;
        return mo.property(index.column()).read(object.data());
    }

    return QVariant();
}

template<class T>
QVariant QpObjectListModel<T>::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Vertical)
        return QVariant();

    if(role == Qt::DisplayRole) {
        QMetaObject mo = T::staticMetaObject;
        return QVariant(mo.property(section).name());
    }

    return QVariant();
}
