#ifndef QPERSISTENCE_CACHEDDATAACCESSOBJECT_H
#define QPERSISTENCE_CACHEDDATAACCESSOBJECT_H

#include <QPersistenceAbstractDataAccessObject.h>

#include <QtCore/QSharedPointer>

template<class T>
class QPersistenceCachedDataAccessObject : public QPersistenceAbstractDataAccessObject
{
public:
    explicit QPersistenceCachedDataAccessObject(QPersistenceAbstractDataAccessObject *source, QObject *parent = 0);

    QPersistenceMetaObject dataSuiteMetaObject() const Q_DECL_OVERRIDE;

    int count() const Q_DECL_OVERRIDE;
    QList<QVariant> allKeys() const Q_DECL_OVERRIDE;
    QList<QObject *> readAllObjects() const Q_DECL_OVERRIDE;
    QObject *createObject() const Q_DECL_OVERRIDE;
    QObject *readObject(const QVariant &key) const Q_DECL_OVERRIDE;
    bool insertObject(QObject *const object) Q_DECL_OVERRIDE;
    bool updateObject(QObject *const object) Q_DECL_OVERRIDE;
    bool removeObject(QObject *const object) Q_DECL_OVERRIDE;

    QList<T *> readAll() const;
    T *create() const;
    T *read(const QVariant &key) const;
    bool insert(T *const object);
    bool update(T *const object);
    bool remove(T *const object);

    void cacheAll();

private:
    mutable QHash<QVariant, QSharedPointer<T> > m_cache;
    QPersistenceAbstractDataAccessObject *m_source;

    mutable int m_cachedCount;
    mutable bool m_cachedAll;

    T *getFromCache(const QVariant &key) const;
    void insertIntoCache(const QVariant &key, T *object) const;
};

#include "cacheddataaccessobject.cpp"

#endif // QPERSISTENCE_CACHEDDATAACCESSOBJECT_H
