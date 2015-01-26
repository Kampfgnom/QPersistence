#ifndef QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H
#define QPERSISTENCE_SQLDATAACCESSOBJECTHELPER_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "sqlquery.h"

class QSqlQuery;
class QpMetaObject;
class QpMetaProperty;
class QpSqlCondition;
class QpSqlQuery;
class QpStorage;

class QpSqlDataAccessObjectHelperData;
class QpSqlDataAccessObjectHelper : public QObject
{
    Q_OBJECT
public:
    ~QpSqlDataAccessObjectHelper();

    explicit QpSqlDataAccessObjectHelper(QpStorage *storage);

    int count(const QpMetaObject &metaObject, const QpSqlCondition &condition) const;
    QList<int> allKeys(const QpMetaObject &metaObject, int skip, int count) const;
    bool readObject(const QpMetaObject &metaObject, const QVariant &key, QObject *object);
    QpSqlQuery readAllObjects(const QpMetaObject &metaObject, int skip, int count, const QpSqlCondition &condition, QList<QpSqlQuery::OrderField> orders);
    QpSqlQuery readObjectsUpdatedAfterRevision(const QpMetaObject &metaObject, int objectRevision);
    bool insertObject(const QpMetaObject &metaObject, QObject *object);
    bool updateObject(const QpMetaObject &metaObject, QObject *object);
    bool removeObject(const QpMetaObject &metaObject, QObject *object);
    bool incrementNumericColumn(QObject *object, const QString &fieldName);
    int latestRevision(const QpMetaObject &metaObject) const;
    int maxPrimaryKey(const QpMetaObject &metaObject) const;

#ifndef QP_NO_GUI
    QPixmap readPixmap(QObject *object, const QString &propertyName);
#endif

    int objectRevision(QObject *object) const;

#ifndef QP_NO_TIMESTAMPS
    double readUpdateTime(QObject *object);
    double readCreationTime(QObject *object);
#endif

    void readQueryIntoObject(const QpSqlQuery &query, const QSqlRecord record, QObject *object);

    int foreignKey(const QpMetaProperty relation, QObject *object);
    QList<int> foreignKeys(const QpMetaProperty relation, QObject *object);

private:
    QExplicitlySharedDataPointer<QpSqlDataAccessObjectHelperData> data;

    void fillValuesIntoQuery(const QpMetaObject &metaObject,
                             const QObject *object,
                             QpSqlQuery &query);

    void selectFields(const QpMetaObject &metaObject,
                      QpSqlQuery &query);

    bool adjustRelationsInDatabase(const QpMetaObject &metaObject, QObject *object);

    QpSqlQuery queryForForeignKeys(const QpMetaProperty &relation);

    QList<QpSqlQuery> queriesThatAdjustOneToManyRelation(const QpMetaProperty &relation, QObject *object);
    QList<QpSqlQuery> queriesThatAdjustOneToOneRelation(const QpMetaProperty &relation, QObject *object);
    QList<QpSqlQuery> queriesThatAdjustToOneRelation(const QpMetaProperty &relation, QObject *object);
    QList<QpSqlQuery> queriesThatAdjustManyToManyRelation(const QpMetaProperty &relation, QObject *object);

};

#endif // SQLDATAACCESSOBJECTHELPER_H
