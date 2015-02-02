#ifndef QPERSISTENCE_THROTTLEDFETCHPROXYMODEL_H
#define QPERSISTENCE_THROTTLEDFETCHPROXYMODEL_H

#include "defines.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QIdentityProxyModel>
#include <QtCore/QElapsedTimer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "model.h"

class QpThrottledFetchProxyModel : public QIdentityProxyModel, public QpModelBase
{
    Q_OBJECT
public:
    explicit QpThrottledFetchProxyModel(QObject *parent = 0);

    int throttle() const;
    void setThrottle(int throttle);

    bool isThrottled() const;

    bool canFetchMore(const QModelIndex &parent) const Q_DECL_OVERRIDE;

private:
    int m_throttle;
    mutable QElapsedTimer m_timer;
};


#endif // QPERSISTENCE_THROTTLEDFETCHPROXYMODEL_H
