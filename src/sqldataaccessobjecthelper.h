#ifndef QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H
#define QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H

#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>

class QSqlQuery;
class QpError;
class QpMetaObject;
class QpMetaProperty;
class QpSqlQuery;

class QpSqlDataAccessObjectHelperPrivate;
class QpSqlDataAccessObjectHelper : public QObject
{
    Q_OBJECT
public:
    ~QpSqlDataAccessObjectHelper();

    static QpSqlDataAccessObjectHelper *forDatabase(const QSqlDatabase &database = QSqlDatabase::database());

    int count(const QpMetaObject &metaObject) const;
    QList<int> allKeys(const QpMetaObject &metaObject, int skip, int count) const;
    bool readObject(const QpMetaObject &metaObject, const QVariant &key, QObject *object);
    QpSqlQuery readAllObjects(const QpMetaObject &metaObject, int skip, int count);
    bool insertObject(const QpMetaObject &metaObject, QObject *object);
    bool updateObject(const QpMetaObject &metaObject, QObject *object);
    bool removeObject(const QpMetaObject &metaObject, QObject *object);
    bool readDatabaseTimes(const QpMetaObject &metaObject, QObject *object);
    void readQueryIntoObject(const QSqlQuery &query,
                             QObject *object);

    QpError lastError() const;

    int foreignKey(const QpMetaProperty relation, QObject *object);
    QList<int> foreignKeys(const QpMetaProperty relation, QObject *object);

private:
    QSharedDataPointer<QpSqlDataAccessObjectHelperPrivate> data;

    explicit QpSqlDataAccessObjectHelper(const QSqlDatabase &database, QObject *parent = 0);

    void setLastError(const QpError &error) const;
    void setLastError(const QSqlQuery &query) const;

    QList<QpSqlQuery> fillValuesIntoQuery(const QpMetaObject &metaObject,
                                          const QObject *object,
                                          QpSqlQuery &queryconst);
    bool adjustRelationsInDatabase(const QpMetaObject &metaObject, QObject *object);

    QList<QpSqlQuery> queriesThatAdjustOneToManyRelation(const QpMetaProperty &relation, QObject *object);
    QList<QpSqlQuery> queriesThatAdjustOneToOneRelation(const QpMetaProperty &relation, QObject *object);
    QList<QpSqlQuery> queriesThatAdjustToOneRelation(const QpMetaProperty &relation, QObject *object);

};

#endif // SQLDATAACCESSOBJECTHELPER_H
