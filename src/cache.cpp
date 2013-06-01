#include "cache.h"
#include <QSharedData>

#include <QQueue>
#include <QDebug>

class QpCacheData : public QSharedData {
public:
    QHash<int, QWeakPointer<QObject> > cache;
    mutable QQueue<QSharedPointer<QObject> > strongCache;
    int strongCacheSize;
};

QpCache::QpCache() : data(new QpCacheData)
{
    data->strongCacheSize = 50;
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

bool QpCache::contains(int id) const
{
    return data->cache.contains(id);
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
    data->strongCache.enqueue(p);

    if(data->strongCacheSize < data->strongCache.size())
        data->strongCache.dequeue();

    return p;
}

QSharedPointer<QObject> QpCache::get(int id) const
{
    QSharedPointer<QObject> p = data->cache.value(id).toStrongRef();
    if(p) {
        int i = 0;
        foreach(QSharedPointer<QObject> object, data->strongCache) {
            if(object.data() == p.data()) {
                data->strongCache.takeAt(i);
                data->strongCache.enqueue(p);
            }
            ++i;
        }
    }

    return p;
}

void QpCache::remove(int id)
{
    QSharedPointer<QObject> p = data->cache.take(id).toStrongRef();
    if(p) {
        int i = 0;
        foreach(QSharedPointer<QObject> object, data->strongCache) {
            if(object.data() == p.data()) {
                data->strongCache.takeAt(i);
            }
            ++i;
        }
    }
}
