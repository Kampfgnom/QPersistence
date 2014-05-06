#ifndef THROTTLEDFETCHPROXYMODEL_H
#define THROTTLEDFETCHPROXYMODEL_H

#include <QIdentityProxyModel>
#include <QElapsedTimer>

class QpThrottledFetchProxyModel : public QIdentityProxyModel
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


#endif // THROTTLEDFETCHPROXYMODEL_H
