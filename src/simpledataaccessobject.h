#ifndef QPERSISTENCE_SIMPLEDATAACCESSOBJECT_H
#define QPERSISTENCE_SIMPLEDATAACCESSOBJECT_H

#include <QDataSuite/abstractdataaccessobject.h>

#include <QDataSuite/metaobject.h>
#include <QtCore/QHash>
#include <QtCore/QVariant>

template<class T>
class QPersistenceSimpleDataAccessObject : public QPersistenceAbstractDataAccessObject
{
public:
    QPersistenceSimpleDataAccessObject(QObject *parent = 0);

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

private:
    QHash<QVariant, T *> m_objects;
    QPersistenceMetaObject m_metaObject;
};

#include "simpledataaccessobject.cpp"

#endif // QPERSISTENCE_SIMPLEDATAACCESSOBJECT_H

