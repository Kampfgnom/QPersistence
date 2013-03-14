#ifndef QPERSISTENCE_PERSISTENTDATAACCESSOBJECT_H
#define QPERSISTENCE_PERSISTENTDATAACCESSOBJECT_H

#include <QPersistenceAbstractDataAccessObject.h>

#include <QPersistence.h>
#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>

class QSqlQuery;
class QPersistenceError;
class QPersistenceSqlDataAccessObjectHelper;

class QPersistencePersistentDataAccessObjectBasePrivate;
class QPersistencePersistentDataAccessObjectBase : public QPersistenceAbstractDataAccessObject
{
    Q_OBJECT
public:
    explicit QPersistencePersistentDataAccessObjectBase(const QMetaObject &metaObject,
                                            const QSqlDatabase &database = QSqlDatabase::database(),
                                            QObject *parent = 0);
    ~QPersistencePersistentDataAccessObjectBase();

    QPersistenceSqlDataAccessObjectHelper *sqlDataAccessObjectHelper() const;
    QPersistenceMetaObject dataSuiteMetaObject() const;

    int count() const Q_DECL_OVERRIDE;
    QList<QVariant> allKeys() const Q_DECL_OVERRIDE;
    QList<QObject *> readAllObjects() const Q_DECL_OVERRIDE;
    QObject *readObject(const QVariant &key) const Q_DECL_OVERRIDE;
    bool insertObject(QObject *const object) Q_DECL_OVERRIDE;
    bool updateObject(QObject *const object) Q_DECL_OVERRIDE;
    bool removeObject(QObject *const object) Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<QPersistencePersistentDataAccessObjectBasePrivate> d;

    Q_DISABLE_COPY(QPersistencePersistentDataAccessObjectBase)
};

template<class T>
class QPersistencePersistentDataAccessObject : public QPersistencePersistentDataAccessObjectBase
{
public:
    explicit QPersistencePersistentDataAccessObject(const QSqlDatabase &database = QSqlDatabase::database(), QObject *parent = 0) :
        QPersistencePersistentDataAccessObjectBase(T::staticMetaObject, database, parent)
    {
    }

    QList<T *> readAll() const { return QPersistence::castList<T>(readAllObjects()); }
    QObject *createObject() const Q_DECL_OVERRIDE { return new T; }
    T *create() const { return static_cast<T *>(createObject()); }
    T *read(const QVariant &key) const { return static_cast<T *>(readObject(key)); }
    bool insert(T *const object) { return insertObject(object); }
    bool update(T *const object) { return updateObject(object); }
    bool remove(T *const object) { return removeObject(object); }
};

#endif // QPERSISTENCE_PERSISTENTDATAACCESSOBJECT_H
