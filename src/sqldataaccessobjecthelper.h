#ifndef QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H
#define QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H

#include <QtCore/QObject>

#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>


class QSqlQuery;
class QPersistenceError;
class QPersistenceMetaObject;
class QPersistenceSqlQuery;
class QPersistencePersistentDataAccessObjectBase;

class QPersistenceSqlDataAccessObjectHelperPrivate;
class QPersistenceSqlDataAccessObjectHelper : public QObject
{
    Q_OBJECT
public:
    ~QPersistenceSqlDataAccessObjectHelper();

    static QPersistenceSqlDataAccessObjectHelper *forDatabase(const QSqlDatabase &database = QSqlDatabase::database());

    int count(const QPersistenceMetaObject &metaObject) const;
    QList<QVariant> allKeys(const QPersistenceMetaObject &metaObject) const;
    bool readObject(const QPersistenceMetaObject &metaObject, const QVariant &key, QObject *object);
    bool insertObject(const QPersistenceMetaObject &metaObject, QObject *object);
    bool updateObject(const QPersistenceMetaObject &metaObject, const QObject *object);
    bool removeObject(const QPersistenceMetaObject &metaObject, const QObject *object);
    bool readRelatedObjects(const QPersistenceMetaObject &metaObject, QObject *object);

    QPersistenceError lastError() const;

private:
    QSharedDataPointer<QPersistenceSqlDataAccessObjectHelperPrivate> d;

    explicit QPersistenceSqlDataAccessObjectHelper(const QSqlDatabase &database, QObject *parent = 0);

    void setLastError(const QPersistenceError &error) const;
    void setLastError(const QSqlQuery &query) const;

    void fillValuesIntoQuery(const QPersistenceMetaObject &metaObject,
                             const QObject *object,
                             QPersistenceSqlQuery &queryconst);
    void readQueryIntoObject(const QSqlQuery &query,
                             QObject *object);
    bool adjustRelations(const QPersistenceMetaObject &metaObject, const QObject *object);
    bool readRelatedObjects(const QPersistenceMetaObject &metaObject,
                            QObject *object,
                            QHash<QString, QHash<QVariant, QObject *> > &alreadyReadObjectsPerTable);

};

#endif // SQLDATAACCESSOBJECTHELPER_H
