#include "cache.h"
#include <QSharedData>

#include <QDebug>
#include <QQueue>

class QpCacheData : public QSharedData {
public:
    QHash<int, QWeakPointer<QObject> > cache;
    mutable QQueue<QSharedPointer<QObject> > strongCache;
    mutable QQueue<QObject *> pointerCache; // is always in sync with strong cache. will be used to keep track of indizes.
    int strongCacheSize;
};

QpCache::QpCache() :
    data(new QpCacheData)
{
    data->strongCacheSize = 50;
}

QpCache::QpCache(const QpCache &rhs) :
    data(rhs.data)
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
    if (p) {
        Q_ASSERT_X(false,
                   Q_FUNC_INFO,
                   QString("The cache already contains an object with the id '%1'")
                   .arg(id).toLatin1());
    }

    p = QSharedPointer<QObject>(object);
    data->cache.insert(id, p.toWeakRef());
    data->strongCache.enqueue(p);
    data->pointerCache.enqueue(p.data());

    if (data->strongCacheSize < data->strongCache.size()) {
        data->strongCache.dequeue();
        data->pointerCache.dequeue();
    }

    return p;
}

QSharedPointer<QObject> QpCache::get(int id) const
{
    QSharedPointer<QObject> p = data->cache.value(id).toStrongRef();
    if (p) {
        int i = data->pointerCache.indexOf(p.data());
        if (i != -1) {
            data->strongCache.removeAt(i);
            data->pointerCache.removeAt(i);
        }
        data->strongCache.enqueue(p);
        data->pointerCache.enqueue(p.data());
    }

    return p;
}

void QpCache::remove(int id)
{
    QSharedPointer<QObject> p = data->cache.take(id).toStrongRef();
    if (p) {
        int i = data->pointerCache.indexOf(p.data());
        if (i != -1) {
            data->strongCache.removeAt(i);
            data->pointerCache.removeAt(i);
        }
    }
}

QList<QSharedPointer<QObject> > QpCache::objects(int skip, int count) const
{
    if (skip > 0)
        return data->strongCache.mid(skip, count);

    return data->strongCache;
}

int QpCache::size() const
{
    return data->strongCache.size();
}

int QpCache::maximumCacheSize() const
{
    return data->strongCacheSize;
}

void QpCache::setMaximumCacheSize(int size)
{
    while (size < data->strongCache.size()) {
        data->strongCache.dequeue();
        data->pointerCache.dequeue();
    }

    data->strongCacheSize = size;
}
