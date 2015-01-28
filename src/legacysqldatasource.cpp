#include "legacysqldatasource.h"

#include "datasourceresult.h"
#include "error.h"
#include "metaproperty.h"
#include "storage.h"
#include "sqlbackend.h"

#include <QMetaMethod>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlRecord>
#include <QThread>

/******************************************************************************
 * QpLegacySqlDatasourceData
 */
class QpLegacySqlDatasourceData : public QSharedData
{
public:
    QSqlDatabase database;

    void selectFields(const QpMetaObject &metaObject, QpSqlQuery &query) const;
    void readQueryIntoResult(QpDatasourceResult *result, QpSqlQuery &query, const QSqlRecord &record, const QpMetaObject &metaObject) const;
    void fillValuesIntoQuery(const QObject *object, QpSqlQuery &query) const;
    int objectRevision(QpDatasourceResult *result, const QObject *object) const;
    void adjustRelationsInDatabase(QpDatasourceResult *result, const QObject *object) const;

private:
    QList<QpSqlQuery> queriesThatAdjustOneToOneRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const;
    QList<QpSqlQuery> queriesThatAdjustOneToManyRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const;
    QList<QpSqlQuery> queriesThatAdjustToOneRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const;
    QList<QpSqlQuery> queriesThatAdjustManyToManyRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const;
};

void QpLegacySqlDatasourceData::selectFields(const QpMetaObject &metaObject, QpSqlQuery &query) const
{
    query.setForwardOnly(true);

    // Select normal fields
    foreach (const QpMetaProperty property, metaObject.simpleProperties()) {
        if (property.isLazy())
            continue;

        QString columnName = property.columnName();
#ifdef QP_FOR_MYSQL
        if (property.metaProperty().isEnumType())
            columnName += "+0";
#endif
        query.addField(columnName);
    }

    // Select some internal fields
    query.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    query.addField(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG);

#ifndef QP_NO_TIMESTAMPS
    query.addField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME);
    query.addField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME);
#endif
#ifndef QP_NO_LOCKS
#pragma message ("QpLegacySqlDatasourceData::selectFields remember to test for isLocksEnabled")
//    if (data->storage->isLocksEnabled())
    query.addField(QpDatabaseSchema::COLUMN_LOCK);
#endif

    // History subselects:

    // We select the revision as a subquery with another subquery, because the needed GROUP BY does not work well with the relations' LEFT JOINs
    // and we also need the `action`-field as a 'content' field as in http://stackoverflow.com/a/7745635.
    // BTW: Other RDBMS wouldn't have allowed this error ;)
    // Me:          Who wants some MySQL specifics??
    // Also me:     HERE ME! MYSQL IS GREAT!!!1!!11elf!
    // Me again:    -.-

    // The resulting query is something along the lines of:
    //    SELECT fields FROM object
    //        LEFT JOIN (
    //            SELECT historyStuff
    //            FROM object_Qp_history
    //            INNER JOIN (
    //                SELECT MAX(revision), historyStuff
    //                FROM object_Qp_history
    //                GROUP BY _Qp_ID) AS history_revision_subselect
    //            ON id AND revision) AS history_subselect
    //        ON id
    //        ORDER BY revision ASC

    const QString historyTable = QString::fromLatin1(QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY).arg(metaObject.tableName());
    const QString revisionSubSubQueryName = "history_revision_subselect";
    const QString revisionSubQueryName = "history_subselect";

    // Sub-Subselect for getting the MAX(revision) grouped by the ID
    QpSqlQuery revisionSubSubQuery(database);
    revisionSubSubQuery.setTable(historyTable);
    revisionSubSubQuery.setTableName(revisionSubSubQueryName);
    revisionSubSubQuery.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    revisionSubSubQuery.addRawField(QString::fromLatin1("MAX(%1) AS %1")
                                    .arg(QpDatabaseSchema::COLUMN_NAME_REVISION));
    revisionSubSubQuery.addGroupBy(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);

    // Subselect for actually getting the correct `action`-field
    QpSqlQuery revisionSubQuery(database);
    revisionSubQuery.setTable(historyTable);
    revisionSubQuery.setTableName(revisionSubQueryName);
    revisionSubQuery.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    revisionSubQuery.addField(QpDatabaseSchema::COLUMN_NAME_REVISION);
    revisionSubQuery.addField(QpDatabaseSchema::COLUMN_NAME_ACTION);
    revisionSubQuery.addJoin("INNER",
                             revisionSubSubQuery,
                             QString::fromLatin1("%1.%3 = %2.%3 AND %1.%4 = %2.%4")
                             .arg(QpSqlQuery::escapeField(revisionSubQueryName))
                             .arg(QpSqlQuery::escapeField(revisionSubSubQueryName))
                             .arg(QpSqlQuery::escapeField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY))
                             .arg(QpSqlQuery::escapeField(QpDatabaseSchema::COLUMN_NAME_REVISION)));

    // Add subselects to query
    query.addJoin("LEFT",
                  revisionSubQuery,
                  QString::fromLatin1("%1 = %2.%3")
                  .arg(query.escapedQualifiedField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY))
                  .arg(revisionSubQueryName)
                  .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY));
    query.addRawField(QpSqlQuery::escapeField(QpDatabaseSchema::COLUMN_NAME_REVISION));

    // Implicitly join to-one related tables
    int tableJoin = 0;
    foreach (const QpMetaProperty relation, metaObject.relationProperties()) {
        if (relation.hasTableForeignKey()) {
            query.addField(relation.columnName());
        }
        else if (relation.isToOneRelationProperty()) {
            QString joinName = QString("join_table_%1").arg(tableJoin++);
            query.addRawField(joinName + "." +
                              QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY +
                              " AS  _Qp_FK_" + relation.name());
            query.addJoin("LEFT", QpSqlQuery::escapeField(relation.tableName()).append(" AS ").append(joinName),
                          joinName + "." + QpSqlQuery::escapeField(relation.columnName()) +
                          " = " + query.escapedQualifiedField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY));
        }
    }
}


void QpLegacySqlDatasourceData::readQueryIntoResult(QpDatasourceResult *result,
                                                    QpSqlQuery &query,
                                                    const QSqlRecord &record,
                                                    const QpMetaObject &metaObject) const
{
    QList<QpDatasourceResult::Record> resultRecords;
    resultRecords.reserve(query.size());

    while (query.next()) {
        QpDatasourceResult::Record resultRecord;
        int fieldCount = record.count();
        resultRecord.metaObject = metaObject.metaObject();
        resultRecord.fields.reserve(fieldCount);

        for (int i = 0; i < fieldCount; ++i) {
            QpDatasourceResult::RecordField currentField;
            currentField.name = record.fieldName(i);
            QMetaProperty metaProperty = query.propertyForIndex(record, &resultRecord.metaObject, i);
            currentField.propertyIndex = -1;
            currentField.value = QVariant();

            if (!metaProperty.isValid()) {
                if (currentField.name == QLatin1String("_Qp_ID")) {
                    resultRecord.primaryKey = query.value(i).toInt();
                    currentField.value = resultRecord.primaryKey;
                }
                else if (currentField.name == QLatin1String("revision")) {
                    currentField.value = query.value(i).toInt();
                }
                else if (!currentField.name.startsWith("_Qp_")) {
                    continue;
                }
            }
            else {
                currentField.propertyIndex = metaProperty.propertyIndex();
                currentField.value = query.value(i);

                if (metaProperty.isFlagType()) {
                    currentField.value = currentField.value.toInt();
                }
                else if (metaProperty.isEnumType()) {
#ifdef QP_FOR_MYSQL
                    currentField.value = metaProperty.enumerator().value(currentField.value.toInt());
#elif defined QP_FOR_SQLITE
                    currentField.value = currentField.value.toInt();
#endif
                }
                else {
                    QMetaType::Type type = static_cast<QMetaType::Type>(currentField.value.userType());
                    currentField.value = QpSqlQuery::variantFromSqlStorableVariant(currentField.value, type);
                }
            }

            resultRecord.fields.append(currentField);
        }

        resultRecords.append(resultRecord);
    }

    result->setRecords(resultRecords);
}

void QpLegacySqlDatasourceData::fillValuesIntoQuery(const QObject *object,
                                                    QpSqlQuery &query) const
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);

    foreach (const QpMetaProperty property, metaObject.simpleProperties()) {
        QVariant value = property.metaProperty().read(object);

        // handle NULL SET or ENUM values
        if (value == QVariant(0)
            && (property.metaProperty().isEnumType()
                || property.metaProperty().isFlagType())) {
            query.addRawField(property.columnName(), "NULL");
        }
        else {
            // Handle ENUMs
            if (property.metaProperty().isEnumType() && !property.metaProperty().isFlagType())
#ifdef QP_FOR_MYSQL
                value = property.metaProperty().enumerator().valueToKey(value.toInt());
#elif QP_FOR_SQLITE
                value = value.toInt();
#endif

            query.addField(property.columnName(), value);
        }

    }
}

int QpLegacySqlDatasourceData::objectRevision(QpDatasourceResult *result, const QObject *object) const
{
    QpSqlQuery query(database);
    QString historyTable = QString::fromLatin1(QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY).arg(QpMetaObject::forObject(object).tableName());
    query.setTable(historyTable);
    query.setLimit(1);
    query.addRawField(QString::fromLatin1("MAX(%1) AS %1").arg(QpDatabaseSchema::COLUMN_NAME_REVISION));
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                        QpCondition::EqualTo,
                                        Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec() || !query.first()) {
        result->setError(query);
        return -1;
    }

    return query.value(0).toInt();
}

void QpLegacySqlDatasourceData::adjustRelationsInDatabase(QpDatasourceResult *result, const QObject *object) const
{
    QList<QpSqlQuery> queries;
    QpMetaObject metaObject = QpMetaObject::forObject(object);

    foreach (const QpMetaProperty property, metaObject.relationProperties()) {
        QpMetaProperty::Cardinality cardinality = property.cardinality();

        if (cardinality == QpMetaProperty::OneToOneCardinality) {
            queries.append(queriesThatAdjustOneToOneRelation(result, property, object));
        }
        else if (cardinality == QpMetaProperty::OneToManyCardinality) {
            queries.append(queriesThatAdjustOneToManyRelation(result, property, object));
        }
        else if (cardinality == QpMetaProperty::ManyToOneCardinality) {
            queries.append(queriesThatAdjustToOneRelation(result, property, object));
        }
        else if (cardinality == QpMetaProperty::ManyToManyCardinality) {
            queries.append(queriesThatAdjustManyToManyRelation(result, property, object));
        }
    }

    if (result->error().isValid())
        return;

    foreach (QpSqlQuery query, queries) {
        if (!query.exec()) {
            result->setError(query);
            return;
        }
    }
}

QList<QpSqlQuery> QpLegacySqlDatasourceData::queriesThatAdjustOneToOneRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const
{
    if (relation.hasTableForeignKey())
        return queriesThatAdjustToOneRelation(result, relation, object);

    QList<QpSqlQuery> queries;
    QVariant primaryKey = Qp::Private::primaryKey(object);
    QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(relation.metaProperty().read(object));

    // Prepare a query, which resets the relation (set old foreign key to NULL)
    // This also adjusts the update time of a previously related object
    QpCondition whereClause = QpCondition(relation.columnName(),
                                          QpCondition::EqualTo,
                                          primaryKey);
    if (relatedObject) {
        whereClause = whereClause && QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                 QpCondition::NotEqualTo,
                                                 Qp::Private::primaryKey(relatedObject.data()));
    }

    QpSqlQuery resetRelationQuery(database);
    resetRelationQuery.setTable(relation.tableName());
    resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
    resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
#endif
    resetRelationQuery.setWhereCondition(whereClause);
    resetRelationQuery.prepareUpdate();
    queries.append(resetRelationQuery);

    if (!relatedObject)
        return queries;

    QVariant relatedPrimary = Qp::Private::primaryKey(relatedObject.data());
    // Prepare actual update
    QpSqlQuery setForeignKeyQuery(database);
    setForeignKeyQuery.setTable(relation.tableName());
    setForeignKeyQuery.addField(relation.columnName(), primaryKey);
#ifndef QP_NO_TIMESTAMPS
    setForeignKeyQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
#endif
    setForeignKeyQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                     QpCondition::EqualTo,
                                                     relatedPrimary)
                                         && QpCondition(QString("%1 IS NULL").arg(relation.columnName())));
    setForeignKeyQuery.prepareUpdate();
    queries.append(setForeignKeyQuery);
    return queries;
}

QList<QpSqlQuery> QpLegacySqlDatasourceData::queriesThatAdjustOneToManyRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const
{
    Q_UNUSED(result);

    QList<QpSqlQuery> queries;
    QVariant primaryKey = Qp::Private::primaryKey(object);

    QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(relation.metaProperty().read(object));

    // Build an OR'd where clause, which matches all now related objects
    QList<QpCondition> relatedObjectsWhereClauses;
    foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
        relatedObjectsWhereClauses.append(QpCondition(QString("%1.%2")
                                                      .arg(relation.tableName())
                                                      .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY),
                                                      QpCondition::EqualTo,
                                                      Qp::Private::primaryKey(relatedObject.data())));
    }
    QpCondition relatedObjectsWhereClause(QpCondition::Or, relatedObjectsWhereClauses);

    // The reset condition matches all objects, which have previously been related with me, but are not now
    QpCondition resetCondition = QpCondition(relation.columnName(),
                                             QpCondition::EqualTo,
                                             primaryKey);
    if (!relatedObjects.isEmpty()) {
        resetCondition = resetCondition && !relatedObjectsWhereClause;
    }

    // Prepare a query, which resets the relation for all objects, which have been related and aren't anymore
    // This also adjusts the update times of these now unrelated objects
    QpSqlQuery resetRelationQuery(database);
    resetRelationQuery.setTable(relation.tableName());
    resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
    resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
#endif
    resetRelationQuery.setWhereCondition(resetCondition);
    resetRelationQuery.prepareUpdate();
    queries.append(resetRelationQuery);

    if (relatedObjects.isEmpty())
        return queries;


    QpCondition newlyRelatedObjectsClause = relatedObjectsWhereClause
                                            && QpCondition(QpCondition::Or, QList<QpCondition>()
                                                           << QpCondition(relation.columnName(),
                                                                          QpCondition::NotEqualTo,
                                                                          primaryKey)
                                                           << QpCondition(QString("%1 IS NULL").arg(relation.columnName())));


#ifndef QP_NO_TIMESTAMPS
    QpCondition relatedObjectsWhereClause2 = relatedObjectsWhereClause;
    relatedObjectsWhereClause2.setBindValuesAsString(true);
    QString updateTimeQueryString = QString("UPDATE %1 AS tableToUpdate"
                                            "\n\tINNER JOIN %2 "
                                            "\n\t\tON %2.%3 = tableToUpdate.%4 "
                                            "\n\tSET tableToUpdate.%5 = %6 "
                                            "\n\tWHERE %7")
                                    .arg(QpSqlQuery::escapeField(relation.metaObject().tableName()))
                                    .arg(QpSqlQuery::escapeField(relation.tableName()))
                                    .arg(QpSqlQuery::escapeField(relation.columnName()))
                                    .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                                    .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
                                    .arg(QpSqlBackend::forDatabase(database)->nowTimestamp())
                                    .arg(relatedObjectsWhereClause2.toSqlClause());

    QpSqlQuery setUpdateTimeOnRelatedObjectsQuery(database);
    setUpdateTimeOnRelatedObjectsQuery.prepare(updateTimeQueryString);
    queries.append(setUpdateTimeOnRelatedObjectsQuery);
#endif


    // Prepare a query, which sets the foreign keys of the related objects to our object's primary key
    QpSqlQuery setForeignKeysQuery(database);
    setForeignKeysQuery.setTable(relation.tableName());
    setForeignKeysQuery.addField(relation.columnName(), primaryKey);
#ifndef QP_NO_TIMESTAMPS
    setForeignKeysQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
#endif
    setForeignKeysQuery.setWhereCondition(newlyRelatedObjectsClause);
    setForeignKeysQuery.prepareUpdate();
    queries.append(setForeignKeysQuery);

    return queries;
}

QList<QpSqlQuery> QpLegacySqlDatasourceData::queriesThatAdjustToOneRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const
{
    QVariant primaryKey = Qp::Private::primaryKey(object);

    QVariant relatedPrimary;
    QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(relation.metaProperty().read(object));
    if (relatedObject) {
        relatedPrimary = Qp::Private::primaryKey(relatedObject.data());
    }

    QpSqlQuery selectPreviouslyRelatedObjectPKQuery(database);
    selectPreviouslyRelatedObjectPKQuery.setTable(relation.tableName());
    selectPreviouslyRelatedObjectPKQuery.addField(relation.columnName());
    selectPreviouslyRelatedObjectPKQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                       QpCondition::EqualTo,
                                                                       primaryKey));
    selectPreviouslyRelatedObjectPKQuery.prepareSelect();
    if (!selectPreviouslyRelatedObjectPKQuery.exec()) {
        result->setError(selectPreviouslyRelatedObjectPKQuery);
        return {};
    }

    QVariant previousRelatedPK;
    if (selectPreviouslyRelatedObjectPKQuery.first())
        previousRelatedPK = selectPreviouslyRelatedObjectPKQuery.value(0);

    if (previousRelatedPK == relatedPrimary)
        return {};

    QList<QpSqlQuery> queries;

#ifndef QP_NO_TIMESTAMPS
    if (!previousRelatedPK.isNull()) {
        // Prepare a query, which adjusts the update time of a previously related object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQueryPreviouslyRelated(database);
        adjustUpdateTimeQueryPreviouslyRelated.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQueryPreviouslyRelated.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
        adjustUpdateTimeQueryPreviouslyRelated.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                             QpCondition::EqualTo,
                                                                             previousRelatedPK));
        adjustUpdateTimeQueryPreviouslyRelated.prepareUpdate();
        queries.append(adjustUpdateTimeQueryPreviouslyRelated);
    }
#endif

    if (!relatedPrimary.isNull()) {
        if (relation.cardinality() == QpMetaProperty::OneToOneCardinality) {
            // Prepare a query, which resets the relation (in my table: set old foreign key to NULL)
            QpSqlQuery resetRelationQuery(database);
            resetRelationQuery.setTable(relation.tableName());
            resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
            resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
#endif
            resetRelationQuery.setWhereCondition(QpCondition(relation.columnName(),
                                                             QpCondition::EqualTo,
                                                             relatedPrimary));
            resetRelationQuery.prepareUpdate();
            queries.append(resetRelationQuery);
        }

#ifndef QP_NO_TIMESTAMPS
        // Prepare a query, which adjusts the update time of the new foreign object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQuery(database);
        adjustUpdateTimeQuery.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
        adjustUpdateTimeQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                            QpCondition::EqualTo,
                                                            relatedPrimary));
        adjustUpdateTimeQuery.prepareUpdate();
        queries.append(adjustUpdateTimeQuery);
#endif
    }

    // Prepare update (in my table) (this might be a SET to NULL)
    QpSqlQuery setForeignKeyQuery(database);
    setForeignKeyQuery.setTable(relation.tableName());
    setForeignKeyQuery.addField(relation.columnName(), relatedPrimary);
#ifndef QP_NO_TIMESTAMPS
    setForeignKeyQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(database)->nowTimestamp());
#endif
    setForeignKeyQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                     QpCondition::EqualTo,
                                                     primaryKey));
    setForeignKeyQuery.prepareUpdate();
    queries.append(setForeignKeyQuery);

    return queries;

}

QList<QpSqlQuery> QpLegacySqlDatasourceData::queriesThatAdjustManyToManyRelation(QpDatasourceResult *result, const QpMetaProperty &relation, const QObject *object) const
{
    Q_UNUSED(result);

    QList<QpSqlQuery> queries;
    QVariant primaryKey = Qp::Private::primaryKey(object);

    QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(relation.metaProperty().read(object));

    // Build an OR'd where clause, which matches all now related objects
    QList<QpCondition> relatedObjectsWhereClauses;
    QList<QpCondition> relatedObjectsWhereClauses2;
    foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
        relatedObjectsWhereClauses.append(QpCondition(relation.reverseRelation().columnName(),
                                                      QpCondition::EqualTo,
                                                      Qp::Private::primaryKey(relatedObject.data())));
        relatedObjectsWhereClauses2.append(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                       QpCondition::EqualTo,
                                                       Qp::Private::primaryKey(relatedObject.data())));
        // TODO: React to 999-clauses bug
    }

    QpCondition relatedObjectsWhereClause(QpCondition::Or, relatedObjectsWhereClauses);
    QpCondition relatedObjectsWhereClause2(QpCondition::Or, relatedObjectsWhereClauses2);
    relatedObjectsWhereClause2.setBindValuesAsString(true);

    // The reset condition matches all objects, which have previously been related with me, but are not now
    QpCondition resetCondition = QpCondition(relation.columnName(),
                                             QpCondition::EqualTo,
                                             primaryKey);
    resetCondition.setTable(relation.tableName());
    if (!relatedObjects.isEmpty()) {
        resetCondition = resetCondition && !relatedObjectsWhereClause;
    }

#ifndef QP_NO_TIMESTAMPS
    QpCondition resetCondition2 = resetCondition;
    resetCondition2.setBindValuesAsString(true);
    // Update the times of now unrelated objects
    QString updatePreviouslyRelatedTimeQueryString = QString("UPDATE %1 AS tableToUpdate"
                                                             "\n\tINNER JOIN %2 "
                                                             "\n\t\tON %2.%3 = tableToUpdate.%4 "
                                                             "\n\tSET tableToUpdate.%5 = %6 "
                                                             "\n\tWHERE %7")
                                                     .arg(QpSqlQuery::escapeField(relation.reverseRelation().metaObject().tableName()))
                                                     .arg(QpSqlQuery::escapeField(relation.tableName()))
                                                     .arg(QpSqlQuery::escapeField(relation.reverseRelation().columnName()))
                                                     .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                                                     .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
                                                     .arg(QpSqlBackend::forDatabase(database)->nowTimestamp())
                                                     .arg(resetCondition2.toSqlClause());

    QpSqlQuery setUpdateTimeOnPreviouslyRelatedObjectsQuery(database);
    setUpdateTimeOnPreviouslyRelatedObjectsQuery.prepare(updatePreviouslyRelatedTimeQueryString);
    queries.append(setUpdateTimeOnPreviouslyRelatedObjectsQuery);
#endif

    // Remove now unrelated relations
    QpSqlQuery removeNowUnrelatedQuery(database);
    removeNowUnrelatedQuery.setTable(relation.tableName());
    removeNowUnrelatedQuery.setWhereCondition(resetCondition);
    removeNowUnrelatedQuery.prepareDelete();
    queries.append(removeNowUnrelatedQuery);

    if (relatedObjects.isEmpty())
        return queries;

#ifndef QP_NO_TIMESTAMPS
    QString updateNewlyRelatedTimeQueryString = QString("UPDATE %1 SET %2 = %3 "
                                                        "WHERE %4 "
                                                        "AND NOT EXISTS (SELECT 1 FROM %5 WHERE %6 = %1.%7 AND %8 = %9)")
                                                .arg(QpSqlQuery::escapeField(relation.reverseRelation().metaObject().tableName()))
                                                .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
                                                .arg(QpSqlBackend::forDatabase(database)->nowTimestamp())
                                                .arg(relatedObjectsWhereClause2.toSqlClause())
                                                .arg(QpSqlQuery::escapeField(relation.tableName()))
                                                .arg(QpSqlQuery::escapeField(relation.reverseRelation().columnName()))
                                                .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                                                .arg(QpSqlQuery::escapeField(relation.columnName()))
                                                .arg(primaryKey.toString());

    QpSqlQuery setUpdateTimeOnRelatedObjectsQuery(database);
    setUpdateTimeOnRelatedObjectsQuery.prepare(updateNewlyRelatedTimeQueryString);
    queries.append(setUpdateTimeOnRelatedObjectsQuery);
#endif

    // Add newly related relations
    foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
        QVariant relatedPK = Qp::Private::primaryKey(relatedObject.data());

        QpSqlQuery createRelationQuery(database);
        createRelationQuery.setOrIgnore(true);
        createRelationQuery.setTable(relation.tableName());
        createRelationQuery.addField(relation.columnName(), primaryKey);
        createRelationQuery.addField(relation.reverseRelation().columnName(), relatedPK);
        createRelationQuery.prepareInsert();
        queries.append(createRelationQuery);
    }

    return queries;
}

/******************************************************************************
 * QpLegacySqlDatasource
 */
QpLegacySqlDatasource::QpLegacySqlDatasource(QObject *parent) :
    QpDatasource(parent),
    data(new QpLegacySqlDatasourceData)
{
}

QpLegacySqlDatasource::~QpLegacySqlDatasource()
{
}

QSqlDatabase QpLegacySqlDatasource::database() const
{
    return data->database;
}

void QpLegacySqlDatasource::setSqlDatabase(const QSqlDatabase &database)
{
    data->database = database;
}

void QpLegacySqlDatasource::count(QpDatasourceResult *result, const QpMetaObject &metaObject, const QpCondition &condition) const
{
    QString q = QString::fromLatin1("SELECT COUNT(*) FROM %1")
                .arg(QpSqlQuery::escapeField(metaObject.tableName()));

    QpCondition c = condition;
    c = QpCondition::notDeletedAnd(c);
    c.setBindValuesAsString(true);

    if (c.isValid())
        q.append(QString(" WHERE %1").arg(c.toSqlClause()));

    QpSqlQuery query(data->database);
    query.prepare(q);

    if (!query.exec() || !query.first()) {
        result->setError(query);
        return;
    }

    result->setIntegerResult(query.value(0).toInt());
}

void QpLegacySqlDatasource::latestRevision(QpDatasourceResult *result, const QpMetaObject &metaObject) const
{
    QpSqlQuery query(data->database);
    if (!query.exec(QString::fromLatin1(
                            "SELECT `AUTO_INCREMENT` "
                            "FROM  INFORMATION_SCHEMA.TABLES "
                            "WHERE TABLE_SCHEMA = '%1' "
                            "AND   TABLE_NAME   = '%2';")
                    .arg(data->database.databaseName())
                    .arg(QString::fromLatin1(QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY).arg(metaObject.tableName())))
        || !query.first()) {
        result->setError(query);
        return;
    }

    result->setIntegerResult(query.value(0).toInt() - 1);
}

void QpLegacySqlDatasource::maxPrimaryKey(QpDatasourceResult *result, const QpMetaObject &metaObject) const
{
    QpSqlQuery query(data->database);
    if (!query.exec(QString::fromLatin1(
                            "SELECT MAX(%1) "
                            "FROM  %2")
                    .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                    .arg(QpSqlQuery::escapeField(metaObject.tableName())))
        || !query.first()) {
        result->setError(query);
        return;
    }

    result->setIntegerResult(query.value(0).toInt() - 1);
}

void QpLegacySqlDatasource::objectByPrimaryKey(QpDatasourceResult *result, const QpMetaObject &metaObject, int primaryKey) const
{
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    data->selectFields(metaObject, query);
    query.setLimit(1);
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                        QpCondition::EqualTo,
                                        primaryKey));
    query.prepareSelect();

    if (!query.exec()) {
        result->setError(query);
        return;
    }

    data->readQueryIntoResult(result, query, query.record(), metaObject);
}

void QpLegacySqlDatasource::objects(QpDatasourceResult *result, const QpMetaObject &metaObject, int skip, int limit, const QpCondition &condition, QList<QpDatasource::OrderField> orders) const
{
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    data->selectFields(metaObject, query);
    query.setWhereCondition(QpCondition::notDeletedAnd(condition));
    query.setLimit(limit);
    query.setSkip(skip);
    query.setForwardOnly(true);
    foreach (QpDatasource::OrderField orderField, orders) {
        query.addOrder(orderField.field, static_cast<QpSqlQuery::Order>(orderField.order));
    }
    query.prepareSelect();

    if (!query.exec()) {
        result->setError(query);
        return;
    }

    data->readQueryIntoResult(result, query, query.record(), metaObject);
}

void QpLegacySqlDatasource::objectsUpdatedAfterRevision(QpDatasourceResult *result, const QpMetaObject &metaObject, int revision) const
{
    QString qualifiedRevisionField = QString::fromLatin1("%1.%2")
                                     .arg(QpSqlQuery::escapeField("history_subselect"))
                                     .arg(QpSqlQuery::escapeField(QpDatabaseSchema::COLUMN_NAME_REVISION));
    objects(result, metaObject, -1, -1,
            QString::fromLatin1("%1 > %2 AND %3 in ('UPDATE', 'MARK_AS_DELETE')")
            .arg(qualifiedRevisionField)
            .arg(revision)
            .arg(QpDatabaseSchema::COLUMN_NAME_ACTION),
            {{qualifiedRevisionField, QpDatasource::Ascending}});
}

void QpLegacySqlDatasource::objectRevision(QpDatasourceResult *result, const QObject *object) const
{
    QpSqlQuery query(data->database);
    QString historyTable = QString::fromLatin1(QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY).arg(QpMetaObject::forObject(object).tableName());
    query.setTable(historyTable);
    query.setLimit(1);
    query.addRawField(QString::fromLatin1("MAX(%1) AS %1").arg(QpDatabaseSchema::COLUMN_NAME_REVISION));
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                        QpCondition::EqualTo,
                                        Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec() || !query.first()) {
        result->setError(query);
        return;
    }

    result->setIntegerResult(data->objectRevision(result, object));
}

void QpLegacySqlDatasource::insertObject(QpDatasourceResult *result, const QObject *object) const
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);

    // Create main INSERT query
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    data->fillValuesIntoQuery(object, query);

#ifndef QP_NO_TIMESTAMPS
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
#endif

    // Insert the object itself
    query.prepareInsert();
    if (!query.exec()) {
        result->setError(query);
        return;
    }

    objectByPrimaryKey(result, metaObject, query.lastInsertId().toInt());
}

void QpLegacySqlDatasource::updateObject(QpDatasourceResult *result, const QObject *object) const
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);
    int primaryKey = Qp::Private::primaryKey(object);

    // Create main UPDATE query
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                        QpCondition::EqualTo,
                                        primaryKey));
    data->fillValuesIntoQuery(object, query);

    query.addField(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG, Qp::Private::isDeleted(object));

#ifndef QP_NO_TIMESTAMPS
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
#endif

    // Insert the object itself
    query.prepareUpdate();
    if (!query.exec()) {
        result->setError(query);
        return;
    }

    // Update related objects
    data->adjustRelationsInDatabase(result, object);
    if (result->error().isValid())
        return;

    objectByPrimaryKey(result, metaObject, primaryKey);
}

void QpLegacySqlDatasource::removeObject(QpDatasourceResult *result, const QObject *object) const
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);

    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                        QpCondition::EqualTo,
                                        Qp::Private::primaryKey(object)));
    query.prepareDelete();

    if (!query.exec()) {
        result->setError(query);
    }
}

void QpLegacySqlDatasource::incrementNumericColumn(QpDatasourceResult *result, const QObject *object, const QString &fieldName) const
{
    static const int TRY_COUNT_MAX = 100;
    int tryCount = 0;

    QpMetaObject mo = QpMetaObject::forObject(object);
    int primaryKey = Qp::Private::primaryKey(object);
    QpSqlQuery query(data->database);
    query.setTable(mo.tableName());
    query.addField(fieldName);
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                        QpCondition::EqualTo,
                                        primaryKey));
    query.prepareincrementNumericColumn();

    do {
        ++tryCount;

        if (query.exec())
            break;

        if (tryCount >= TRY_COUNT_MAX || query.lastError().nativeErrorCode() != QLatin1String("1213")) {
            result->setError(query);
            return;
        }

        qDebug() << Q_FUNC_INFO << QString::fromLatin1("MySQL found a deadlock. Re-trying transaction (try %1 of %2).")
                .arg(tryCount)
                .arg(TRY_COUNT_MAX);
        QThread::usleep(100);
    } while (true);

    objectByPrimaryKey(result, mo, primaryKey);
}
