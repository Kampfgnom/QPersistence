#include <QDataSuite/simpledataaccessobject.h>

#include <QDataSuite/metaproperty.h>
#include <QDataSuite/metaobject.h>
#include <QDataSuite/error.h>

#include <QDebug>



template<class T>
QPersistenceSimpleDataAccessObject<T>::QPersistenceSimpleDataAccessObject(QObject *parent) :
    QPersistenceAbstractDataAccessObject(parent),
    m_metaObject(QPersistenceMetaObject::metaObject(T::staticMetaObject))
{}

template<class T>
QPersistenceMetaObject QPersistenceSimpleDataAccessObject<T>::dataSuiteMetaObject() const
{
    return m_metaObject;
}

template<class T>
int QPersistenceSimpleDataAccessObject<T>::count() const
{
    return m_objects.size();
}

template<class T>
QList<QVariant> QPersistenceSimpleDataAccessObject<T>::allKeys() const
{
    resetLastError();
    return m_objects.keys();
}

template<class T>
QList<T *> QPersistenceSimpleDataAccessObject<T>::readAll() const
{
    resetLastError();
    return m_objects.values();
}

template<class T>
QList<QObject *> QPersistenceSimpleDataAccessObject<T>::readAllObjects() const
{
    QList<QObject *> result;
    Q_FOREACH(T *object, readAll()) result.append(object);
    return result;
}

template<class T>
T *QPersistenceSimpleDataAccessObject<T>::create() const
{
    resetLastError();
    return new T;
}

template<class T>
QObject *QPersistenceSimpleDataAccessObject<T>::createObject() const
{
    return create();
}

template<class T>
T *QPersistenceSimpleDataAccessObject<T>::read(const QVariant &key) const
{
    resetLastError();
    int type = m_metaObject.primaryKeyProperty().type();
    Q_ASSERT(key.canConvert(type));

    QVariant keyVariant(key);
    keyVariant.convert(type);

    return m_objects.value(keyVariant);
}

template<class T>
QObject *QPersistenceSimpleDataAccessObject<T>::readObject(const QVariant &key) const
{
    return read(key);
}

template<class T>
bool QPersistenceSimpleDataAccessObject<T>::insert(T * const object)
{
    resetLastError();
    QVariant key = m_metaObject.primaryKeyProperty().read(object);
    if(m_objects.contains(key)) {
        setLastError(QPersistenceError("An object with this key already exists.",
                                       QPersistenceError::StorageError));
        return false;
    }

    m_objects.insert(key, object);
    emit objectInserted(object);
    return true;
}

template<class T>
bool QPersistenceSimpleDataAccessObject<T>::insertObject(QObject *const object)
{
    T * const t = qobject_cast<T * const>(object);
    Q_ASSERT(t);
    return insert(t);
}

template<class T>
bool QPersistenceSimpleDataAccessObject<T>::update(T *const object)
{
    resetLastError();
    bool ok = m_objects.contains(m_metaObject.primaryKeyProperty().read(object));
    if(ok)
        emit objectUpdated(object);
    return ok;
}

template<class T>
bool QPersistenceSimpleDataAccessObject<T>::updateObject(QObject *const object)
{
    T *t = qobject_cast<T *>(object);
    Q_ASSERT(t);
    return update(t);
}

template<class T>
bool QPersistenceSimpleDataAccessObject<T>::remove(T *const object)
{
    resetLastError();
    QVariant key = m_metaObject.primaryKeyProperty().read(object);
    bool ok = m_objects.contains(key);
    if(ok) {
        m_objects.remove(key);
        emit objectRemoved(object);
    }
    return ok;
}

template<class T>
bool QPersistenceSimpleDataAccessObject<T>::removeObject(QObject *const object)
{
    T *t = qobject_cast<T *>(object);
    Q_ASSERT(t);
    return remove(t);
}


