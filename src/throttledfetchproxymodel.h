#ifndef THROTTLEDFETCHPROXYMODEL_H
#define THROTTLEDFETCHPROXYMODEL_H

#include "defines.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QIdentityProxyModel>
#include <QtCore/QElapsedTimer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpThrottledFetchProxyModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    explicit QpThrottledFetchProxyModel(QObject *parent = 0);

    int throttle() const;
    void setThrottle(int throttle);

    bool isThrottled() const;

    bool canFetchMore(const QModelIndex &parent) const Q_DECL_OVERRIDE;

    QModelIndex indexForObject(QSharedPointer<QObject> object) const;
    QSharedPointer<QObject> objectByIndex(const QModelIndex &index) const;

private:
    int m_throttle;
    mutable QElapsedTimer m_timer;
};


#endif // THROTTLEDFETCHPROXYMODEL_H
