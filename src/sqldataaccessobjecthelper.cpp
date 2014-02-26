#include "sqldataaccessobjecthelper.h"

#include "dataaccessobject.h"
#include "databaseschema.h"
#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqlcondition.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDebug>
#include <QMetaProperty>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QVariant>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpSqlDataAccessObjectHelperPrivate : public QSharedData
{
public:
    QpSqlDataAccessObjectHelperPrivate() :
        QSharedData()
    {}

    QSqlDatabase database;
    mutable QpError lastError;

    static QHash<QString, QpSqlDataAccessObjectHelper *> helpersForConnection;
};

typedef QHash<QString, QpSqlDataAccessObjectHelper *> HashStringToDaoHelper;
QP_DEFINE_STATIC_LOCAL(HashStringToDaoHelper, HelpersForConnection)

QpSqlDataAccessObjectHelper::QpSqlDataAccessObjectHelper(const QSqlDatabase &database, QObject *parent) :
    QObject(parent),
    data(new QpSqlDataAccessObjectHelperPrivate)
{
    data->database = database;
}

QpSqlDataAccessObjectHelper::~QpSqlDataAccessObjectHelper()
{
}

QpSqlDataAccessObjectHelper *QpSqlDataAccessObjectHelper::forDatabase(const QSqlDatabase &database)
{
    QpSqlDataAccessObjectHelper* asd = new QpSqlDataAccessObjectHelper(database, Qp::Private::GlobalGuard());

    if (!HelpersForConnection()->contains(database.connectionName()))
        HelpersForConnection()->insert(database.connectionName(),
                                                                        asd);

    return HelpersForConnection()->value(database.connectionName());
}

int QpSqlDataAccessObjectHelper::count(const QpMetaObject &metaObject) const
{
    QpSqlQuery query(data->database);
    query.prepare(QString("SELECT COUNT(*) FROM %1")
                  .arg(metaObject.tableName()));

    if (!query.exec()
            || !query.first()
            || query.lastError().isValid()) {
        setLastError(query);
        return 0;
    }

    return query.value(0).toInt();
}

QList<int> QpSqlDataAccessObjectHelper::allKeys(const QpMetaObject &metaObject, int skip, int count) const
{
    QpSqlQuery query(data->database);
    query.clear();
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    query.setCount(count);
    query.setSkip(skip);
    query.prepareSelect();

    QString filter = metaObject.sqlFilter();
    if (!filter.isEmpty())
        query.setWhereCondition(QpSqlCondition(filter));

    QList<int> result;
    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return result;
    }

    while (query.next()) {
        result.append(query.value(0).toInt());
    }

    return result;
}

bool QpSqlDataAccessObjectHelper::readObject(const QpMetaObject &metaObject,
                                             const QVariant &key,
                                             QObject *object)
{
    Q_ASSERT(object);
    Q_ASSERT(!key.isNull());

    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setCount(1);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           key));
    query.prepareSelect();

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    if(!query.first())
        return false;

    readQueryIntoObject(query, query.record(), object);
    return object;
}

QpSqlQuery QpSqlDataAccessObjectHelper::readAllObjects(const QpMetaObject &metaObject, int skip, int count, const QpSqlCondition &condition)
{
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(condition);
    query.setCount(count);
    query.setSkip(skip);
    query.setForwardOnly(true);
    query.prepareSelect();

    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
    }

    return query;
}

bool QpSqlDataAccessObjectHelper::insertObject(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    // Create main INSERT query
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    fillValuesIntoQuery(metaObject, object, query);
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());

    // Insert the object itself
    query.prepareInsert();
    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    Qp::Private::setPrimaryKey(object, query.lastInsertId().toInt());
    double time = readCreationTime(metaObject, object);
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, time);
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME, time);

    return true;
}

bool QpSqlDataAccessObjectHelper::updateObject(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    // Create main UPDATE query
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    fillValuesIntoQuery(metaObject, object, query);
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());

    // Insert the object itself
    query.prepareUpdate();
    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    object->setProperty(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, readUpdateTime(metaObject, object));

    // Update related objects
    return adjustRelationsInDatabase(metaObject, object);
}

void QpSqlDataAccessObjectHelper::fillValuesIntoQuery(const QpMetaObject &metaObject,
                                                                   const QObject *object,
                                                                   QpSqlQuery &query)
{
    // Add simple properties
    foreach (const QpMetaProperty property, metaObject.simpleProperties()) {
        if(!property.metaProperty().isEnumType()) {
            query.addField(property.columnName(), property.metaProperty().read(object));
            continue;
        }

        // Handle enums
        QVariant value = QpSqlBackend::forDatabase(data->database)->propertyToEnum(property.metaProperty().read(object), property.metaProperty());
        query.addField(property.columnName(), value);
    }
}

void QpSqlDataAccessObjectHelper::readQueryIntoObject(const QpSqlQuery &query, const QSqlRecord record, QObject *object)
{
    int fieldCount = record.count();
    for (int i = 0; i < fieldCount; ++i) {

        QMetaProperty property = query.propertyForIndex(record, object->metaObject(), i);
        if(!property.isValid())
            continue;

        QVariant value = query.value(i);

        if(property.isEnumType()) {
            value = QpSqlBackend::forDatabase(data->database)->enumToProperty(value, property);
        }
        else {
            QMetaType::Type type = static_cast<QMetaType::Type>(property.userType());
            value = QpSqlQuery::variantFromSqlStorableVariant(value, type);
        }

        property.write(object, value);
    }

    object->setProperty(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY, query.value(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY));
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, query.value(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME));
}

bool QpSqlDataAccessObjectHelper::adjustRelationsInDatabase(const QpMetaObject &metaObject, QObject *object)
{
    QList<QpSqlQuery> queries;

    foreach (const QpMetaProperty property, metaObject.relationProperties()) {
        QpMetaProperty::Cardinality cardinality = property.cardinality();

        if (cardinality == QpMetaProperty::OneToOneCardinality) {
            queries.append(queriesThatAdjustOneToOneRelation(property, object));
        }
        else if (cardinality == QpMetaProperty::OneToManyCardinality) {
            queries.append(queriesThatAdjustOneToManyRelation(property, object));
        }
        else if (cardinality == QpMetaProperty::ManyToOneCardinality) {
            queries.append(queriesThatAdjustToOneRelation(property, object));
        }
        else if (cardinality == QpMetaProperty::ManyToManyCardinality) {
            queries.append(queriesThatAdjustManyToManyRelation(property, object));
        }
    }

    foreach (QpSqlQuery query, queries) {
        if (!query.exec()
                || query.lastError().isValid()) {
            setLastError(query);
            return false;
        }
    }

    return true;
}

QList<QpSqlQuery> QpSqlDataAccessObjectHelper::queriesThatAdjustOneToOneRelation(const QpMetaProperty &relation, QObject *object)
{
    if(relation.hasTableForeignKey())
        return queriesThatAdjustToOneRelation(relation, object);

    QList<QpSqlQuery> queries;
    QVariant primaryKey = Qp::Private::primaryKey(object);
    QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(relation.metaProperty().read(object));

    // Prepare a query, which resets the relation (set old foreign key to NULL)
    // This also adjusts the update time of a previously related object
    QpSqlCondition whereClause = QpSqlCondition(relation.columnName(),
                                                QpSqlCondition::EqualTo,
                                                primaryKey);
    if(relatedObject) {
        whereClause = whereClause && QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                    QpSqlCondition::NotEqualTo,
                                                    Qp::primaryKey(relatedObject));
    }

    QpSqlQuery resetRelationQuery(data->database);
    resetRelationQuery.setTable(relation.tableName());
    resetRelationQuery.addField(relation.columnName(), QVariant());
    resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
    resetRelationQuery.setWhereCondition(whereClause);
    resetRelationQuery.prepareUpdate();
    queries.append(resetRelationQuery);

    if(!relatedObject)
        return queries;
    QVariant relatedPrimary = Qp::primaryKey(relatedObject);

    // Prepare actual update
    QpSqlQuery setForeignKeyQuery(data->database);
    setForeignKeyQuery.setTable(relation.tableName());
    setForeignKeyQuery.addField(relation.columnName(), primaryKey);
    setForeignKeyQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
    setForeignKeyQuery.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                        QpSqlCondition::EqualTo,
                                                        relatedPrimary)
                                         && QpSqlCondition(QString("%1 IS NULL").arg(relation.columnName())));
    setForeignKeyQuery.prepareUpdate();
    queries.append(setForeignKeyQuery);
    return queries;
}

QList<QpSqlQuery> QpSqlDataAccessObjectHelper::queriesThatAdjustOneToManyRelation(const QpMetaProperty &relation, QObject *object)
{
    QList<QpSqlQuery> queries;
    QVariant primaryKey = Qp::Private::primaryKey(object);

    QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(relation.metaProperty().read(object));

    // Build an OR'd where clause, which matches all now related objects
    QList<QpSqlCondition> relatedObjectsWhereClauses;
    foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
        relatedObjectsWhereClauses.append(QpSqlCondition(QString("%1.%2")
                                                         .arg(relation.tableName())
                                                         .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY),
                                                         QpSqlCondition::EqualTo,
                                                         Qp::Private::primaryKey(relatedObject.data())));
    }
    QpSqlCondition relatedObjectsWhereClause(QpSqlCondition::Or, relatedObjectsWhereClauses);

    // The reset condition matches all objects, which have previously been related with me, but are not now
    QpSqlCondition resetCondition = QpSqlCondition(relation.columnName(),
                                                   QpSqlCondition::EqualTo,
                                                   primaryKey);
    if(!relatedObjects.isEmpty()) {
        resetCondition = resetCondition && !relatedObjectsWhereClause;
    }

    // Prepare a query, which resets the relation for all objects, which have been related and aren't anymore
    // This also adjusts the update times of these now unrelated objects
    QpSqlQuery resetRelationQuery(data->database);
    resetRelationQuery.setTable(relation.tableName());
    resetRelationQuery.addField(relation.columnName(), QVariant());
    resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
    resetRelationQuery.setWhereCondition(resetCondition);
    resetRelationQuery.prepareUpdate();
    queries.append(resetRelationQuery);

    if(relatedObjects.isEmpty())
        return queries;


    QpSqlCondition newlyRelatedObjectsClause = relatedObjectsWhereClause
            && QpSqlCondition(QpSqlCondition::Or, QList<QpSqlCondition>()
                              << QpSqlCondition(relation.columnName(),
                                                QpSqlCondition::NotEqualTo,
                                                primaryKey)
                              << QpSqlCondition(QString("%1 IS NULL").arg(relation.columnName())));

    QpSqlCondition relatedObjectsWhereClause2 = relatedObjectsWhereClause;
    relatedObjectsWhereClause2.setBindValuesAsString(true);
    QString updateTimeQueryString = QString("UPDATE %1"
                                            "\n\tINNER JOIN %2 "
                                            "\n\t\tON %2.%3 = %1.%4 "
                                            "\n\tSET %1.%5 = %6 "
                                            "\n\tWHERE %7")
            .arg(relation.metaObject().tableName())
            .arg(relation.tableName())
            .arg(relation.columnName())
            .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
            .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
            .arg(QpSqlBackend::forDatabase(data->database)->nowTimestamp())
            .arg(relatedObjectsWhereClause2.toWhereClause());

    QpSqlQuery setUpdateTimeOnRelatedObjectsQuery(data->database);
    setUpdateTimeOnRelatedObjectsQuery.prepare(updateTimeQueryString);
    queries.append(setUpdateTimeOnRelatedObjectsQuery);


    // Prepare a query, which sets the foreign keys of the related objects to our object's primary key
    QpSqlQuery setForeignKeysQuery(data->database);
    setForeignKeysQuery.setTable(relation.tableName());
    setForeignKeysQuery.addField(relation.columnName(), primaryKey);
    setForeignKeysQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
    setForeignKeysQuery.setWhereCondition(newlyRelatedObjectsClause);
    setForeignKeysQuery.prepareUpdate();
    queries.append(setForeignKeysQuery);

    return queries;
}

QList<QpSqlQuery> QpSqlDataAccessObjectHelper::queriesThatAdjustToOneRelation(const QpMetaProperty &relation, QObject *object)
{
    QList<QpSqlQuery> queries;
    QVariant primaryKey = Qp::Private::primaryKey(object);

    QVariant relatedPrimary;
    QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(relation.metaProperty().read(object));
    if (relatedObject) {
        relatedPrimary = Qp::primaryKey(relatedObject);
    }

    QpSqlQuery selectPreviouslyRelatedObjectPKQuery(data->database);
    selectPreviouslyRelatedObjectPKQuery.setTable(relation.tableName());
    selectPreviouslyRelatedObjectPKQuery.addField(relation.columnName());
    selectPreviouslyRelatedObjectPKQuery.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                          QpSqlCondition::EqualTo,
                                                                          primaryKey));
    selectPreviouslyRelatedObjectPKQuery.prepareSelect();
    if(!selectPreviouslyRelatedObjectPKQuery.exec()
            || selectPreviouslyRelatedObjectPKQuery.lastError().isValid()) {
        setLastError(selectPreviouslyRelatedObjectPKQuery);
        return queries;
    }

    QVariant previousRelatedPK;
    if(selectPreviouslyRelatedObjectPKQuery.size() == 1
            && selectPreviouslyRelatedObjectPKQuery.first())
        previousRelatedPK = selectPreviouslyRelatedObjectPKQuery.value(0);

    if(previousRelatedPK == relatedPrimary)
        return queries;

    if(!previousRelatedPK.isNull()) {
        // Prepare a query, which adjusts the update time of a previously related object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQueryPreviouslyRelated(data->database);
        adjustUpdateTimeQueryPreviouslyRelated.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQueryPreviouslyRelated.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
        adjustUpdateTimeQueryPreviouslyRelated.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                                QpSqlCondition::EqualTo,
                                                                                previousRelatedPK));
        adjustUpdateTimeQueryPreviouslyRelated.prepareUpdate();
        queries.append(adjustUpdateTimeQueryPreviouslyRelated);
    }

    if(!relatedPrimary.isNull()) {
        if(relation.cardinality() == QpMetaProperty::OneToOneCardinality) {
            // Prepare a query, which resets the relation (in my table: set old foreign key to NULL)
            QpSqlQuery resetRelationQuery(data->database);
            resetRelationQuery.setTable(relation.tableName());
            resetRelationQuery.addField(relation.columnName(), QVariant());
            resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
            resetRelationQuery.setWhereCondition(QpSqlCondition(relation.columnName(),
                                                                QpSqlCondition::EqualTo,
                                                                relatedPrimary));
            resetRelationQuery.prepareUpdate();
            queries.append(resetRelationQuery);
        }

        // Prepare a query, which adjusts the update time of the new foreign object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQuery(data->database);
        adjustUpdateTimeQuery.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
        adjustUpdateTimeQuery.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                               QpSqlCondition::EqualTo,
                                                               relatedPrimary));
        adjustUpdateTimeQuery.prepareUpdate();
        queries.append(adjustUpdateTimeQuery);
    }

    // Prepare update (in my table) (this might be a SET to NULL)
    QpSqlQuery setForeignKeyQuery(data->database);
    setForeignKeyQuery.setTable(relation.tableName());
    setForeignKeyQuery.addField(relation.columnName(), relatedPrimary);
    setForeignKeyQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
    setForeignKeyQuery.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                        QpSqlCondition::EqualTo,
                                                        primaryKey));
    setForeignKeyQuery.prepareUpdate();
    queries.append(setForeignKeyQuery);

    return queries;

}

QList<QpSqlQuery> QpSqlDataAccessObjectHelper::queriesThatAdjustManyToManyRelation(const QpMetaProperty &relation, QObject *object)
{
    QList<QpSqlQuery> queries;
    QVariant primaryKey = Qp::Private::primaryKey(object);

    QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(relation.metaProperty().read(object));

    // Build an OR'd where clause, which matches all now related objects
    QList<QpSqlCondition> relatedObjectsWhereClauses;
    QList<QpSqlCondition> relatedObjectsWhereClauses2;
    foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
        relatedObjectsWhereClauses.append(QpSqlCondition(QString("%1.%2")
                                                         .arg(relation.tableName())
                                                         .arg(relation.reverseRelation().columnName()),
                                                         QpSqlCondition::EqualTo,
                                                         Qp::Private::primaryKey(relatedObject.data())));
        relatedObjectsWhereClauses2.append(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                         QpSqlCondition::EqualTo,
                                                         Qp::Private::primaryKey(relatedObject.data())));
        // TODO: React to 999-clauses bug
    }

    QpSqlCondition relatedObjectsWhereClause(QpSqlCondition::Or, relatedObjectsWhereClauses);
    QpSqlCondition relatedObjectsWhereClause2(QpSqlCondition::Or, relatedObjectsWhereClauses2);
    relatedObjectsWhereClause2.setBindValuesAsString(true);

    // The reset condition matches all objects, which have previously been related with me, but are not now
    QpSqlCondition resetCondition = QpSqlCondition(relation.columnName(),
                                                   QpSqlCondition::EqualTo,
                                                   primaryKey);
    if(!relatedObjects.isEmpty()) {
        resetCondition = resetCondition && !relatedObjectsWhereClause;
    }

    QpSqlCondition resetCondition2 = resetCondition;
    resetCondition2.setBindValuesAsString(true);
    // Update the times of now unrelated objects
    QString updatePreviouslyRelatedTimeQueryString = QString("UPDATE %1"
                                            "\n\tINNER JOIN %2 "
                                            "\n\t\tON %2.%3 = %1.%4 "
                                            "\n\tSET %1.%5 = %6 "
                                            "\n\tWHERE %7")
            .arg(relation.reverseRelation().metaObject().tableName())
            .arg(relation.tableName())
            .arg(relation.reverseRelation().columnName())
            .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
            .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
            .arg(QpSqlBackend::forDatabase(data->database)->nowTimestamp())
            .arg(resetCondition2.toWhereClause());

    QpSqlQuery setUpdateTimeOnPreviouslyRelatedObjectsQuery(data->database);
    setUpdateTimeOnPreviouslyRelatedObjectsQuery.prepare(updatePreviouslyRelatedTimeQueryString);
    queries.append(setUpdateTimeOnPreviouslyRelatedObjectsQuery);

    // Remove now unrelated relations
    QpSqlQuery removeNowUnrelatedQuery(data->database);
    removeNowUnrelatedQuery.setTable(relation.tableName());
    removeNowUnrelatedQuery.setWhereCondition(resetCondition);
    removeNowUnrelatedQuery.prepareDelete();
    queries.append(removeNowUnrelatedQuery);

    if (relatedObjects.isEmpty())
        return queries;

    QString updateNewlyRelatedTimeQueryString = QString("UPDATE %1 SET %2 = %3 "
                                                        "WHERE %4 "
                                                        "AND NOT EXISTS (SELECT 1 FROM %5 WHERE %6 = %1.%7 AND %8 = %9)")
            .arg(relation.reverseRelation().metaObject().tableName())
            .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
            .arg(QpSqlBackend::forDatabase(data->database)->nowTimestamp())
            .arg(relatedObjectsWhereClause2.toWhereClause())
            .arg(relation.tableName())
            .arg(relation.reverseRelation().columnName())
            .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
            .arg(relation.columnName())
            .arg(primaryKey.toString());

    QpSqlQuery setUpdateTimeOnRelatedObjectsQuery(data->database);
    setUpdateTimeOnRelatedObjectsQuery.prepare(updateNewlyRelatedTimeQueryString);
    queries.append(setUpdateTimeOnRelatedObjectsQuery);

    // Add newly related relations
    foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
        QVariant relatedPK = Qp::Private::primaryKey(relatedObject.data());

        QpSqlQuery createRelationQuery(data->database);
        createRelationQuery.setOrIgnore(true);
        createRelationQuery.setTable(relation.tableName());
        createRelationQuery.addField(relation.columnName(), primaryKey);
        createRelationQuery.addField(relation.reverseRelation().columnName(), relatedPK);
        createRelationQuery.prepareInsert();
        queries.append(createRelationQuery);
    }

    return queries;
}

bool QpSqlDataAccessObjectHelper::removeObject(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareDelete();

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    return true;
}

double QpSqlDataAccessObjectHelper::readUpdateTime(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setCount(1);
    query.addField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return -1.0;
    }

    if(!query.first())
        return -1.0;

    return query.value(0).toDouble();
}

double QpSqlDataAccessObjectHelper::readCreationTime(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setCount(1);
    query.addField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return -1.0;
    }

    if(!query.first())
        return -1.0;

    return query.value(0).toDouble();
}

QpError QpSqlDataAccessObjectHelper::lastError() const
{
    return data->lastError;
}

int QpSqlDataAccessObjectHelper::foreignKey(const QpMetaProperty relation, QObject *object)
{
    QList<int> keys = foreignKeys(relation, object);
    if (keys.isEmpty())
        return 0;

    Q_ASSERT(keys.size() == 1);

    return keys.first();
}

QList<int> QpSqlDataAccessObjectHelper::foreignKeys(const QpMetaProperty relation, QObject *object)
{
    QString foreignColumn;
    QString keyColumn;
    QString sortColumn;
    int key = Qp::Private::primaryKey(object);

    QpMetaProperty::Cardinality cardinality = relation.cardinality();

    if (cardinality == QpMetaProperty::OneToManyCardinality
            || cardinality == QpMetaProperty::OneToOneCardinality) {
        keyColumn = relation.reverseRelation().columnName();
        foreignColumn = QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY;

        if (relation.hasTableForeignKey()) {
            qSwap(keyColumn, foreignColumn);
        }

        sortColumn = foreignColumn;
    }
    else if (cardinality == QpMetaProperty::ManyToOneCardinality) {
        keyColumn = QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY;
        foreignColumn = relation.reverseRelation().columnName();
    }
    else if (cardinality == QpMetaProperty::ManyToManyCardinality) {
        keyColumn = relation.columnName();
        foreignColumn = relation.reverseRelation().columnName();
        sortColumn = QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY;
    }

    Q_ASSERT(!foreignColumn.isEmpty());
    Q_ASSERT(!keyColumn.isEmpty());

    QpSqlQuery query(data->database);
    query.setTable(relation.tableName());
    query.setWhereCondition(QpSqlCondition(keyColumn,
                                           QpSqlCondition::EqualTo,
                                           key));
    query.addField(foreignColumn);
    query.setForwardOnly(true);
    if (!sortColumn.isEmpty())
        query.addOrder(sortColumn);
    query.prepareSelect();

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return QList<int>();
    }

    bool ok = true;
    QList<int> keys;
    keys.reserve(query.size());
    while (query.next()) {
        int currentKey = query.value(0).toInt(&ok);
        if (ok)
            keys.append(currentKey);
    }

    return keys;
}

void QpSqlDataAccessObjectHelper::setLastError(const QpError &error) const
{
    data->lastError = error;
    Qp::Private::setLastError(error);
    qWarning() << qPrintable(error.text());
    qFatal("Aborting due to SQL errors!");
}

void QpSqlDataAccessObjectHelper::setLastError(const QSqlQuery &query) const
{
    setLastError(QpError(QString("%1: %2")
                         .arg(query.lastError().text())
                         .arg(query.executedQuery()), QpError::SqlError));
}


