#include "tst_cachetest.h"

#include "../src/cache.h"

CacheTest::CacheTest()
{
}

void CacheTest::initTestCase()
{
}

void CacheTest::cleanupTestCase()
{
}

void CacheTest::testBasics()
{
    QpCache cache;
    QString name1 = "name1";
    QObject *object = new QObject();
    object->setObjectName(name1);
    int id1 = 123;

    {
        QSharedPointer<QObject> strongRef = cache.insert(id1, object);
        QVERIFY(strongRef.data() == object);
        QVERIFY(cache.contains(id1));

        QSharedPointer<QObject> strongRef2 = cache.get(id1);
        QVERIFY(strongRef2.data() == object);
    }

    QVERIFY(cache.contains(id1));

    {
        QSharedPointer<QObject> strongRef = cache.get(id1);
        QVERIFY(strongRef.data() == object);
        QVERIFY(strongRef->objectName() == name1);
    }
}

void CacheTest::testRemove()
{
    QpCache cache;
    QObject *object = new QObject();
    int id1 = 123;

    QWeakPointer<QObject> weakRef;

    {
        QSharedPointer<QObject> strongRef = cache.insert(id1, object);
        weakRef = strongRef.toWeakRef();
        QVERIFY(cache.contains(id1));
        QVERIFY(weakRef.toStrongRef());
        cache.remove(id1);
        QVERIFY(!cache.contains(id1));
        QVERIFY(weakRef.toStrongRef());
    }

    QVERIFY(!weakRef.toStrongRef());
}

void CacheTest::testMaximumCacheSize()
{
    QpCache cache;
    int cacheSize = 10;
    cache.setMaximumCacheSize(cacheSize);

    QWeakPointer<QObject> weakRef = cache.insert(0, new QObject()).toWeakRef();

    QWeakPointer<QObject> weakRef4;
    QWeakPointer<QObject> weakRef5;

    for(int i = 1; i < cacheSize; ++i) {
        QWeakPointer<QObject> weak = cache.insert(i, new QObject()).toWeakRef();
        QVERIFY(weakRef.toStrongRef());

        if(i == 4) {
            weakRef4 = weak;
        }
        if(i == 5) {
            weakRef5 = weak;
        }
    }

    cache.insert(10, new QObject());
    QVERIFY(!weakRef.toStrongRef());

    cache.setMaximumCacheSize(6);

    QVERIFY(!weakRef4.toStrongRef());
    QVERIFY(weakRef5.toStrongRef());
}

void CacheTest::testSize()
{
    QpCache cache;
    int cacheSize = 10;
    cache.setMaximumCacheSize(cacheSize);

    for(int i = 1; i < cacheSize; ++i) {
        cache.insert(i, new QObject());
        QVERIFY(cache.size() == i);
    }

    for(int i = cacheSize; i < cacheSize * 2; ++i) {
        cache.insert(i, new QObject());
        QVERIFY(cache.size() == cacheSize);
    }
}

void CacheTest::setCacheOrder()
{
    QpCache cache;
    int cacheSize = 10;
    cache.setMaximumCacheSize(cacheSize);

    QWeakPointer<QObject> weakRef5;

    for(int i = 1; i < cacheSize; ++i) {
        QWeakPointer<QObject> weak = cache.insert(i, new QObject()).toWeakRef();

        if(i == 5) {
            weakRef5 = weak;
        }
    }

    QWeakPointer<QObject> weakRef4 = cache.get(4);

    cache.insert(10, new QObject());
    cache.insert(11, new QObject());
    cache.insert(12, new QObject());
    cache.insert(13, new QObject());
    cache.insert(14, new QObject());

    QVERIFY(weakRef4.toStrongRef());
    QVERIFY(!weakRef5.toStrongRef());
}
