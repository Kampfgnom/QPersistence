#ifndef OBJECTLISTMODEL_H
#define OBJECTLISTMODEL_H

#include "abstractobjectlistmodel.h"

template<class T>
class QpObjectListModel : public QpAbstractObjectListModel<T>
{
public:
    explicit QpObjectListModel(QObject *parent = 0);
    
    int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

signals:
    
public slots:
    
};

#include "objectlistmodel.cpp"

#endif // OBJECTLISTMODEL_H
