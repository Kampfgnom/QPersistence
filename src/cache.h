#ifndef QPERSISTENCE_CACHE_H
#define QPERSISTENCE_CACHE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedPointer>
#include <QtCore/QExplicitlySharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpCacheData;
class QpCache
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

    int maximumCacheSize() const;
    void setMaximumCacheSize(int size);

private:
    QExplicitlySharedDataPointer<QpCacheData> data;
};

#endif // QPERSISTENCE_CACHE_H
