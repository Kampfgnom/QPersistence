#include "cache.h"
#include <QSharedData>

#include "private.h"

#include <QLinkedList>

class QpCacheData : public QSharedData {
public:
    QHash<int, QWeakPointer<QObject> > weakCacheById;
    mutable QLinkedList<QSharedPointer<QObject> > strongCache;

    // is always in sync with strong cache. will be used to keep track of indizes,
    // because indexOf(QObject*) is much faster than indexOf(QSharedPointer)
    mutable QLinkedList<QObject *> pointerCache;

    int strongCacheSize;

    void adjustQueue(QSharedPointer<QObject> accessedObject) const;
};

void QpCacheData::adjustQueue(QSharedPointer<QObject> accessedObject) const
{
    if (pointerCache.removeOne(accessedObject.data())) {
        Q_ASSERT(strongCache.removeOne(accessedObject));
    }
    strongCache.append(accessedObject);
    pointerCache.append(accessedObject.data());
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

    data->strongCache.append(p);
    data->pointerCache.append(p.data());

    if (data->strongCacheSize < data->strongCache.size()) {
        data->strongCache.takeFirst();
        data->pointerCache.takeFirst();
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
        if (data->pointerCache.removeOne(p.data())) {
            Q_ASSERT(data->strongCache.removeOne(p));
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
        data->strongCache.takeFirst();
        data->pointerCache.takeFirst();
    }

    data->strongCacheSize = size;
}
