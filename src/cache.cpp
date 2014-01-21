#include "cache.h"
#include <QSharedData>

#include "private.h"

#include <QDebug>
#include <QQueue>

class QpCacheData : public QSharedData {
public:
    QHash<int, QWeakPointer<QObject> > weakCacheById;
    mutable QQueue<QSharedPointer<QObject> > strongCache;

    // is always in sync with strong cache. will be used to keep track of indizes,
    // because indexOf(QObject*) is much faster than indexOf(QSharedPointer)
    mutable QQueue<QObject *> pointerCache;

    int strongCacheSize;

    void adjustQueue(QSharedPointer<QObject> accessedObject) const;
};

void QpCacheData::adjustQueue(QSharedPointer<QObject> accessedObject) const
{
    int i = pointerCache.indexOf(accessedObject.data());
    if (i != -1) {
        strongCache.removeAt(i);
        pointerCache.removeAt(i);
    }
    strongCache.enqueue(accessedObject);
    pointerCache.enqueue(accessedObject.data());
}

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
    return data->weakCacheById.contains(id);
}

QSharedPointer<QObject> QpCache::insert(int id, QObject *object)
{
    QSharedPointer<QObject> p = data->weakCacheById.value(id).toStrongRef();
    if (p) {
        Q_ASSERT_X(false,
                   Q_FUNC_INFO,
                   QString("The cache already contains an object with the id '%1'")
                   .arg(id).toLatin1());
    }

    p = QSharedPointer<QObject>(object);
    QWeakPointer<QObject> weak = p.toWeakRef();
    data->weakCacheById.insert(id, weak);

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
    QSharedPointer<QObject> p = data->weakCacheById.value(id).toStrongRef();
    if (p) {
        data->adjustQueue(p);
    }

    return p;
}

void QpCache::remove(int id)
{
    QSharedPointer<QObject> p = data->weakCacheById.take(id).toStrongRef();
    if (p) {
        int i = data->pointerCache.indexOf(p.data());
        if (i != -1) {
            data->strongCache.removeAt(i);
            data->pointerCache.removeAt(i);
        }
    }
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
