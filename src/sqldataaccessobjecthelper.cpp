#include "sqldataaccessobjecthelper.h"

#include "dataaccessobject.h"
#include "databaseschema.h"
#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "condition.h"
#include "sqlquery.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDebug>
#include <QMetaProperty>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QThread>
#include <QVariant>
#ifndef QP_NO_GUI
#   include <QPixmap>
#endif
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


/******************************************************************************
 * QpSqlDataAccessObjectHelperPrivate
 */
class QpSqlDataAccessObjectHelperData : public QSharedData
{
public:
    QpSqlDataAccessObjectHelperData() :
        QSharedData()
    {
    }

    QpStorage *storage;
    QHash<QpMetaProperty, QpSqlQuery> queriesForRelation;
};


/******************************************************************************
 * QpSqlDataAccessObjectHelper
 */
QpSqlDataAccessObjectHelper::QpSqlDataAccessObjectHelper(QpStorage *storage) :
    data(new QpSqlDataAccessObjectHelperData)
{
    data->storage = storage;
}

QpSqlDataAccessObjectHelper::~QpSqlDataAccessObjectHelper()
{
}

int QpSqlDataAccessObjectHelper::count(const QpMetaObject &metaObject, const QpCondition &condition) const
{
    QpSqlQuery query(data->storage->database());
    QString q = QString("SELECT COUNT(*) FROM %1")
                .arg(QpSqlQuery::escapeField(metaObject.tableName()));

    QpCondition c = condition;
    c = QpCondition::notDeletedAnd(c);
    c.setBindValuesAsString(true);

    if (c.isValid())
        q.append(QString(" WHERE %1").arg(c.toSqlClause()));

    query.prepare(q);

    if (!query.exec() || !query.first()) {
        data->storage->setLastError(query);
        return 0;
    }

    return query.value(0).toInt();
}

bool QpSqlDataAccessObjectHelper::readObject(const QpMetaObject &metaObject,
                                             const QVariant &key,
                                             QObject *object)
{
    Q_ASSERT(object);
    Q_ASSERT(!key.isNull());

    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    selectFields(metaObject, query);
    query.setLimit(1);
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpCondition::EqualTo,
                                           key));
    query.prepareSelect();

    if (!query.exec()) {
        data->storage->setLastError(query);
        return false;
    }

    if (!query.first())
        return false;

    readQueryIntoObject(query, query.record(), object);
    return object;
}

QpSqlQuery QpSqlDataAccessObjectHelper::readAllObjects(const QpMetaObject &metaObject, int skip, int count, const QpCondition &condition, QList<QpSqlQuery::OrderField> orders)
{
    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    selectFields(metaObject, query);
    query.setWhereCondition(QpCondition::notDeletedAnd(condition));
    query.setLimit(count);
    query.setSkip(skip);
    query.setForwardOnly(true);
    foreach (QpSqlQuery::OrderField orderField, orders) {
        query.addOrder(orderField.field, orderField.order);
    }
    query.prepareSelect();

    if (!query.exec()) {
        data->storage->setLastError(query);
    }

    return query;
}

QpSqlQuery QpSqlDataAccessObjectHelper::readObjectsUpdatedAfterRevision(const QpMetaObject &metaObject, int revision)
{
    QString table = metaObject.tableName();
    QString qualifiedRevisionField = QString::fromLatin1("%1.%2")
                                     .arg(QpSqlQuery::escapeField("history_subselect"))
                                     .arg(QpSqlQuery::escapeField(QpDatabaseSchema::COLUMN_NAME_REVISION));

    QpSqlQuery query(data->storage->database());
    query.setTable(table);
    selectFields(metaObject, query);
    query.setWhereCondition(QString::fromLatin1("%1 > %2 AND %3 in ('UPDATE', 'MARK_AS_DELETE')")
                            .arg(qualifiedRevisionField)
                            .arg(revision)
                            .arg(QpDatabaseSchema::COLUMN_NAME_ACTION));
    query.addOrder(qualifiedRevisionField, QpSqlQuery::Ascending);
    query.setForwardOnly(true);
    query.prepareSelect();

    if (!query.exec()) {
        data->storage->setLastError(query);
    }

    return query;
}

bool QpSqlDataAccessObjectHelper::insertObject(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    if (!data->storage->beginTransaction())
        return false;

    // Create main INSERT query
    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    fillValuesIntoQuery(metaObject, object, query);

#ifndef QP_NO_TIMESTAMPS
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif

    // Insert the object itself
    query.prepareInsert();
    if (!query.exec()) {
        data->storage->setLastError(query);
        return false;
    }

    Qp::Private::setPrimaryKey(object, query.lastInsertId().toInt());

    int revision = objectRevision(object);
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_REVISION, revision);

    return data->storage->commitOrRollbackTransaction();
}

bool QpSqlDataAccessObjectHelper::updateObject(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    if (!data->storage->beginTransaction())
        return false;

    // Create main UPDATE query
    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    fillValuesIntoQuery(metaObject, object, query);

    query.addField(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG, Qp::Private::isDeleted(object));

#ifndef QP_NO_TIMESTAMPS
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif

    // Insert the object itself
    query.prepareUpdate();
    if (!query.exec()) {
        data->storage->setLastError(query);
        return false;
    }

    // Update related objects
    adjustRelationsInDatabase(metaObject, object);

    int revision = objectRevision(object);
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_REVISION, revision);

    return data->storage->commitOrRollbackTransaction();
}

void QpSqlDataAccessObjectHelper::fillValuesIntoQuery(const QpMetaObject &metaObject,
                                                      const QObject *object,
                                                      QpSqlQuery &query)
{
    // Add simple properties
    foreach (const QpMetaProperty property, metaObject.simpleProperties()) {
        QVariant value = property.metaProperty().read(object);
        if (value == QVariant(0)
            && (property.metaProperty().isEnumType()
                || property.metaProperty().isFlagType())) {
            query.addRawField(property.columnName(), "NULL");
        }
        else {
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

void QpSqlDataAccessObjectHelper::selectFields(const QpMetaObject &metaObject, QpSqlQuery &query)
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
    if (data->storage->isLocksEnabled())
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
    QpSqlQuery revisionSubSubQuery(data->storage->database());
    revisionSubSubQuery.setTable(historyTable);
    revisionSubSubQuery.setTableName(revisionSubSubQueryName);
    revisionSubSubQuery.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    revisionSubSubQuery.addRawField(QString::fromLatin1("MAX(%1) AS %1")
                                    .arg(QpDatabaseSchema::COLUMN_NAME_REVISION));
    revisionSubSubQuery.addGroupBy(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);

    // Subselect for actually getting the correct `action`-field
    QpSqlQuery revisionSubQuery(data->storage->database());
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

void QpSqlDataAccessObjectHelper::readQueryIntoObject(const QpSqlQuery &query,
                                                      const QSqlRecord record,
                                                      QObject *object)
{
    int fieldCount = record.count();
    for (int i = 0; i < fieldCount; ++i) {
        QString fieldName = record.fieldName(i);
        QMetaProperty property = query.propertyForIndex(record, object->metaObject(), i);
        if (!property.isValid()) {
            if (fieldName.startsWith("_Qp_")
                || fieldName == QLatin1String("revision"))
                object->setProperty(fieldName.toLatin1(), query.value(i));
            continue;
        }

        QVariant value = query.value(i);

        if (property.isFlagType()) {
            value = value.toInt();
        }
        else if (property.isEnumType()) {
#ifdef QP_FOR_MYSQL
            value = property.enumerator().value(value.toInt());
#elif defined QP_FOR_SQLITE
            value = value.toInt();
#endif
        }
        else {
            QMetaType::Type type = static_cast<QMetaType::Type>(property.userType());
            value = QpSqlQuery::variantFromSqlStorableVariant(value, type);
        }

        property.write(object, value);
    }
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
        if (!query.exec()) {
            data->storage->setLastError(query);
            return false;
        }
    }

    return true;
}

QList<QpSqlQuery> QpSqlDataAccessObjectHelper::queriesThatAdjustOneToOneRelation(const QpMetaProperty &relation, QObject *object)
{
    if (relation.hasTableForeignKey())
        return queriesThatAdjustToOneRelation(relation, object);

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

    QpSqlQuery resetRelationQuery(data->storage->database());
    resetRelationQuery.setTable(relation.tableName());
    resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
    resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
    resetRelationQuery.setWhereCondition(whereClause);
    resetRelationQuery.prepareUpdate();
    queries.append(resetRelationQuery);

    if (!relatedObject)
        return queries;

    QVariant relatedPrimary = Qp::Private::primaryKey(relatedObject.data());
    // Prepare actual update
    QpSqlQuery setForeignKeyQuery(data->storage->database());
    setForeignKeyQuery.setTable(relation.tableName());
    setForeignKeyQuery.addField(relation.columnName(), primaryKey);
#ifndef QP_NO_TIMESTAMPS
    setForeignKeyQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
    setForeignKeyQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                        QpCondition::EqualTo,
                                                        relatedPrimary)
                                         && QpCondition(QString("%1 IS NULL").arg(relation.columnName())));
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
    QpSqlQuery resetRelationQuery(data->storage->database());
    resetRelationQuery.setTable(relation.tableName());
    resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
    resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
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
                                    .arg(QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp())
                                    .arg(relatedObjectsWhereClause2.toSqlClause());

    QpSqlQuery setUpdateTimeOnRelatedObjectsQuery(data->storage->database());
    setUpdateTimeOnRelatedObjectsQuery.prepare(updateTimeQueryString);
    queries.append(setUpdateTimeOnRelatedObjectsQuery);
#endif


    // Prepare a query, which sets the foreign keys of the related objects to our object's primary key
    QpSqlQuery setForeignKeysQuery(data->storage->database());
    setForeignKeysQuery.setTable(relation.tableName());
    setForeignKeysQuery.addField(relation.columnName(), primaryKey);
#ifndef QP_NO_TIMESTAMPS
    setForeignKeysQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
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
        relatedPrimary = Qp::Private::primaryKey(relatedObject.data());
    }

    QpSqlQuery selectPreviouslyRelatedObjectPKQuery(data->storage->database());
    selectPreviouslyRelatedObjectPKQuery.setTable(relation.tableName());
    selectPreviouslyRelatedObjectPKQuery.addField(relation.columnName());
    selectPreviouslyRelatedObjectPKQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                          QpCondition::EqualTo,
                                                                          primaryKey));
    selectPreviouslyRelatedObjectPKQuery.prepareSelect();
    if (!selectPreviouslyRelatedObjectPKQuery.exec()) {
        data->storage->setLastError(selectPreviouslyRelatedObjectPKQuery);
        return queries;
    }

    QVariant previousRelatedPK;
    if (selectPreviouslyRelatedObjectPKQuery.first())
        previousRelatedPK = selectPreviouslyRelatedObjectPKQuery.value(0);

    if (previousRelatedPK == relatedPrimary)
        return queries;

#ifndef QP_NO_TIMESTAMPS
    if (!previousRelatedPK.isNull()) {
        // Prepare a query, which adjusts the update time of a previously related object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQueryPreviouslyRelated(data->storage->database());
        adjustUpdateTimeQueryPreviouslyRelated.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQueryPreviouslyRelated.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
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
            QpSqlQuery resetRelationQuery(data->storage->database());
            resetRelationQuery.setTable(relation.tableName());
            resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
            resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
            resetRelationQuery.setWhereCondition(QpCondition(relation.columnName(),
                                                                QpCondition::EqualTo,
                                                                relatedPrimary));
            resetRelationQuery.prepareUpdate();
            queries.append(resetRelationQuery);
        }

#ifndef QP_NO_TIMESTAMPS
        // Prepare a query, which adjusts the update time of the new foreign object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQuery(data->storage->database());
        adjustUpdateTimeQuery.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
        adjustUpdateTimeQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                               QpCondition::EqualTo,
                                                               relatedPrimary));
        adjustUpdateTimeQuery.prepareUpdate();
        queries.append(adjustUpdateTimeQuery);
#endif
    }

    // Prepare update (in my table) (this might be a SET to NULL)
    QpSqlQuery setForeignKeyQuery(data->storage->database());
    setForeignKeyQuery.setTable(relation.tableName());
    setForeignKeyQuery.addField(relation.columnName(), relatedPrimary);
#ifndef QP_NO_TIMESTAMPS
    setForeignKeyQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
    setForeignKeyQuery.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                        QpCondition::EqualTo,
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
                                                     .arg(QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp())
                                                     .arg(resetCondition2.toSqlClause());

    QpSqlQuery setUpdateTimeOnPreviouslyRelatedObjectsQuery(data->storage->database());
    setUpdateTimeOnPreviouslyRelatedObjectsQuery.prepare(updatePreviouslyRelatedTimeQueryString);
    queries.append(setUpdateTimeOnPreviouslyRelatedObjectsQuery);
#endif

    // Remove now unrelated relations
    QpSqlQuery removeNowUnrelatedQuery(data->storage->database());
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
                                                .arg(QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp())
                                                .arg(relatedObjectsWhereClause2.toSqlClause())
                                                .arg(QpSqlQuery::escapeField(relation.tableName()))
                                                .arg(QpSqlQuery::escapeField(relation.reverseRelation().columnName()))
                                                .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                                                .arg(QpSqlQuery::escapeField(relation.columnName()))
                                                .arg(primaryKey.toString());

    QpSqlQuery setUpdateTimeOnRelatedObjectsQuery(data->storage->database());
    setUpdateTimeOnRelatedObjectsQuery.prepare(updateNewlyRelatedTimeQueryString);
    queries.append(setUpdateTimeOnRelatedObjectsQuery);
#endif

    // Add newly related relations
    foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
        QVariant relatedPK = Qp::Private::primaryKey(relatedObject.data());

        QpSqlQuery createRelationQuery(data->storage->database());
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

    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareDelete();

    if (!query.exec()) {
        data->storage->setLastError(query);
        return false;
    }

    return true;
}

bool QpSqlDataAccessObjectHelper::incrementNumericColumn(QObject *object, const QString &fieldName)
{
    static const int TRY_COUNT_MAX = 100;
    int tryCount = 0;
    do {
        ++tryCount;
        if (!data->storage->beginTransaction())
            continue;
        QpMetaObject mo = QpMetaObject::forObject(object);
        QpSqlQuery query(data->storage->database());

        query.setTable(mo.tableName());
        query.addField(fieldName);
        query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                               QpCondition::EqualTo,
                                               Qp::Private::primaryKey(object)));
        query.prepareincrementNumericColumn();

        if (!query.exec()) {
            data->storage->database().rollback();

            if (tryCount < TRY_COUNT_MAX && query.lastError().nativeErrorCode() == QLatin1String("1213")) {
                qDebug() << Q_FUNC_INFO << QString::fromLatin1("MySQL found a deadlock. Re-trying transaction (try %1 of %2).")
                        .arg(tryCount)
                        .arg(TRY_COUNT_MAX);
                QThread::usleep(100);
                continue;
            }

            data->storage->setLastError(query);
            return false;
        }

        if (!data->storage->commitOrRollbackTransaction())
            continue;

        return true;
    } while (true);
}

int QpSqlDataAccessObjectHelper::latestRevision(const QpMetaObject &metaObject) const
{
    QpSqlQuery query(data->storage->database());
    if (!query.exec(QString::fromLatin1(
                            "SELECT `AUTO_INCREMENT` "
                            "FROM  INFORMATION_SCHEMA.TABLES "
                            "WHERE TABLE_SCHEMA = '%1' "
                            "AND   TABLE_NAME   = '%2';")
                    .arg(data->storage->database().databaseName())
                    .arg(QString::fromLatin1(QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY).arg(metaObject.tableName())))
        || !query.first()) {
        data->storage->setLastError(query);
        return -1;
    }

    return query.value(0).toInt() - 1;
}

int QpSqlDataAccessObjectHelper::objectRevision(QObject *object) const
{
    Q_ASSERT(object);

    QpSqlQuery query(data->storage->database());
    QString historyTable = QString::fromLatin1(QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY).arg(QpMetaObject::forObject(object).tableName());
    query.setTable(historyTable);
    query.setLimit(1);
    query.addRawField(QString::fromLatin1("MAX(%1) AS %1").arg(QpDatabaseSchema::COLUMN_NAME_REVISION));
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec()) {
        data->storage->setLastError(query);
        return -1;
    }

    if (!query.first())
        return -1;

    return query.value(0).toInt();
}

int QpSqlDataAccessObjectHelper::maxPrimaryKey(const QpMetaObject &metaObject) const
{
    QpSqlQuery query(data->storage->database());
    if (!query.exec(QString::fromLatin1(
                            "SELECT MAX(%1) "
                            "FROM  %2")
                    .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                    .arg(QpSqlQuery::escapeField(metaObject.tableName())))
        || !query.first()) {
        data->storage->setLastError(query);
        return -1;
    }

    return query.value(0).toInt() - 1;
}

#ifndef QP_NO_GUI
QPixmap QpSqlDataAccessObjectHelper::readPixmap(QObject *object, const QString &propertyName)
{
    QpSqlQuery query(data->storage->database());
    QpMetaObject mo = QpMetaObject::forObject(object);
    query.setTable(mo.tableName());
    query.setLimit(1);
    query.addField(propertyName);
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec() || !query.first()) {
        data->storage->setLastError(query);
        return QPixmap();
    }

    QMetaType::Type type = static_cast<QMetaType::Type>(mo.metaProperty(propertyName).metaProperty().userType());
    return QpSqlQuery::variantFromSqlStorableVariant(query.value(0), type).value<QPixmap>();
}
#endif

#ifndef QP_NO_TIMESTAMPS
double QpSqlDataAccessObjectHelper::readUpdateTime(QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->storage->database());
    query.setTable(QpMetaObject::forObject(object).tableName());
    query.setLimit(1);
    query.addField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME);
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec()) {
        data->storage->setLastError(query);
        return -1.0;
    }

    if (!query.first())
        return -1.0;

    return query.value(0).toDouble();
}

double QpSqlDataAccessObjectHelper::readCreationTime(QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->storage->database());
    query.setTable(QpMetaObject::forObject(object).tableName());
    query.setLimit(1);
    query.addField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME);
    query.setWhereCondition(QpCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec()) {
        data->storage->setLastError(query);
        return -1.0;
    }

    if (!query.first())
        return -1.0;

    return query.value(0).toDouble();
}
#endif

int QpSqlDataAccessObjectHelper::foreignKey(const QpMetaProperty relation, QObject *object)
{
    QList<int> keys = foreignKeys(relation, object);
    if (keys.isEmpty())
        return 0;

    Q_ASSERT(keys.size() == 1);

    return keys.first();
}

QpSqlQuery QpSqlDataAccessObjectHelper::queryForForeignKeys(const QpMetaProperty &relation)
{
    if (data->queriesForRelation.contains(relation))
        return data->queriesForRelation.value(relation);

    QString foreignColumn;
    QString keyColumn;
    QString sortColumn;

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

    QpSqlQuery query(data->storage->database());
    query.setTable(relation.tableName());
    query.addField(foreignColumn);
    query.setForwardOnly(true);
    QpCondition c(keyColumn, QpCondition::EqualTo, ":keyColumn");
    c.setBindValuesAsString(true);

    if (cardinality != QpMetaProperty::ManyToManyCardinality)
        c = QpCondition::notDeletedAnd(c);

    query.setWhereCondition(c);
    query.addOrder(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
    if (!sortColumn.isEmpty())
        query.addOrder(sortColumn);
    query.prepareSelect();
    data->queriesForRelation.insert(relation, query);
    return query;
}

QList<int> QpSqlDataAccessObjectHelper::foreignKeys(const QpMetaProperty relation, QObject *object)
{
    QpSqlQuery query = queryForForeignKeys(relation);

    int pk = Qp::Private::primaryKey(object);
    query.bindValue(QString(":keyColumn"), QVariant(pk));

    if (!query.exec()) {
        data->storage->setLastError(query);
        return QList<int>();
    }

    bool ok = true;
    QList<int> keys;

    while (query.next()) {
        int currentKey = query.value(0).toInt(&ok);
        if (ok)
            keys.append(currentKey);
    }

    return keys;
}


