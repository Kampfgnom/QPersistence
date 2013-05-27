#include "cache.h"
#include <QSharedData>

class QpCacheData : public QSharedData {
public:
    QHash<int, QWeakPointer<QObject> > cache;
};

QpCache::QpCache() : data(new QpCacheData)
{
}

QpCache::QpCache(const QpCache &rhs) : data(rhs.data)
{
}

QpCache &QpCache::operator=(const QpCache &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QpCache::~QpCache()
{
}

QSharedPointer<QObject> QpCache::insert(int id, QObject *object)
{
    QSharedPointer<QObject> p = data->cache.value(id);
    if(p) {
        Q_ASSERT_X(false,
                   Q_FUNC_INFO,
                   QString("The cache already contains an object with the id '%1'")
                   .arg(id).toLatin1());
    }

    p = QSharedPointer<QObject>(object);
    data->cache.insert(id, p.toWeakRef());
    return p;
}

QSharedPointer<QObject> QpCache::get(int id) const
{
    return data->cache.value(id).toStrongRef();
}

void QpCache::remove(int id)
{
    data->cache.remove(id);
}
