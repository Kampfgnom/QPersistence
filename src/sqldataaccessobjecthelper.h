#ifndef QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H
#define QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H

#include <QtCore/QObject>

#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>


class QSqlQuery;
class QpError;
class QpMetaObject;
class QpSqlQuery;
class QpMetaProperty;

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
    bool readAllObjects(const QpMetaObject &metaObject, QList<QObject *> objects, int skip, int count);
    bool insertObject(const QpMetaObject &metaObject, QObject *object);
    bool updateObject(const QpMetaObject &metaObject, QObject *object);
    bool removeObject(const QpMetaObject &metaObject, QObject *object);
//    bool readRelatedObjects(const QpMetaObject &metaObject, QObject *object);

    QpError lastError() const;

    int foreignKey(const QpMetaProperty relation, QObject *object);
    QList<int> foreignKeys(const QpMetaProperty relation, QObject *object);

private:
    QSharedDataPointer<QpSqlDataAccessObjectHelperPrivate> d;

    explicit QpSqlDataAccessObjectHelper(const QSqlDatabase &database, QObject *parent = 0);

    void setLastError(const QpError &error) const;
    void setLastError(const QSqlQuery &query) const;

    void fillValuesIntoQuery(const QpMetaObject &metaObject,
                             const QObject *object,
                             QpSqlQuery &queryconst);
    void readQueryIntoObject(const QSqlQuery &query,
                             QObject *object);
    bool adjustRelationsInDatabase(const QpMetaObject &metaObject, QObject *object);
//    bool readRelatedObjects(const QpMetaObject &metaObject,
//                            QObject *object,
//                            QHash<QString, QHash<QVariant, QObject *> > &alreadyReadObjectsPerTable);

};

#endif // SQLDATAACCESSOBJECTHELPER_H
