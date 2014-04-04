#include "cache.h"

#include "private.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
#include <QLinkedList>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpCacheData : public QSharedData {
public:                      
    int strongCacheSize;
    QHash<int, QWeakPointer<QObject> > weakCacheById;
    mutable QLinkedList<QSharedPointer<QObject> > strongCache;

    void adjustQueue(QSharedPointer<QObject> accessedObject) const;
};

void QpCacheData::adjustQueue(QSharedPointer<QObject> accessedObject) const
{
    strongCache.append(accessedObject);

    if (strongCacheSize < strongCache.size()) {
        strongCache.takeFirst();
    }
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

    if (data->strongCacheSize < data->strongCache.size()) {
        data->strongCache.takeFirst();
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
        data->strongCache.removeOne(p);
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
    }

    data->strongCacheSize = size;
}
