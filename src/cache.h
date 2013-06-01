#ifndef CACHE_H
#define CACHE_H

#include <QObject>

#include <QSharedPointer>
#include <QSharedDataPointer>

class QpCacheData;
class QpCache : public QObject
{
public:
    QpCache();
    QpCache(const QpCache &);
    QpCache &operator=(const QpCache &);
    ~QpCache();
    
    bool contains(int id) const;
    QSharedPointer<QObject> insert(int id, QObject *object);
    QSharedPointer<QObject> get(int id) const;
    void remove(int id);

private:
    QSharedDataPointer<QpCacheData> data;
};

#endif // CACHE_H
