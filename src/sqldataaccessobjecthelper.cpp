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
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDebug>
#include <QMetaProperty>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QVariant>
#include <QSqlField>
#include <QThread>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpSqlDataAccessObjectHelperPrivate : public QSharedData
{
public:
    QpSqlDataAccessObjectHelperPrivate() :
        QSharedData()
    {}

    QpStorage *storage;
};

QpSqlDataAccessObjectHelper::QpSqlDataAccessObjectHelper(QpStorage *storage) :
    QObject(storage),
    data(new QpSqlDataAccessObjectHelperPrivate)
{
    data->storage = storage;
}

QpSqlDataAccessObjectHelper::~QpSqlDataAccessObjectHelper()
{
}

int QpSqlDataAccessObjectHelper::count(const QpMetaObject &metaObject) const
{
    QpSqlQuery query(data->storage->database());
    query.prepare(QString("SELECT COUNT(*) FROM %1")
                  .arg(QpSqlQuery::escapeField(metaObject.tableName())));

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
    QpSqlQuery query(data->storage->database());
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

    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    selectFields(metaObject, query);
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
    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    selectFields(metaObject, query);
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

QpSqlQuery QpSqlDataAccessObjectHelper::readObjectsUpdatedAfterRevision(const QpMetaObject &metaObject, int revision)
{
    QString table = metaObject.tableName();
    QString historyTable = QString::fromLatin1(QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY).arg(table);
    QString qualifiedRevisionField = QString::fromLatin1("%1.%2")
            .arg(historyTable)
            .arg(QpDatabaseSchema::COLUMN_NAME_REVISION);

    QpSqlQuery query(data->storage->database());
    query.setTable(table);
    selectFields(metaObject, query);
    query.addRawField(qualifiedRevisionField);
    query.addJoin("LEFT",
                  historyTable,
                  QString::fromLatin1("%1.%2 = %3.%2")
                  .arg(QpSqlQuery::escapeField(table))
                  .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                  .arg(historyTable));
    query.setWhereCondition(QString::fromLatin1("%1 > %2 AND %3 in ('UPDATE', 'MARK_AS_DELETE')")
                            .arg(qualifiedRevisionField)
                            .arg(revision)
                            .arg(QpDatabaseSchema::COLUMN_NAME_ACTION));
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
    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    fillValuesIntoQuery(metaObject, object, query);

#ifndef QP_NO_TIMESTAMPS
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif

    // Insert the object itself
    query.prepareInsert();
    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    Qp::Private::setPrimaryKey(object, query.lastInsertId().toInt());

#ifndef QP_NO_TIMESTAMPS
    double time = readCreationTime(object);
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME, time);
#endif

    return true;
}

bool QpSqlDataAccessObjectHelper::updateObject(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    // Create main UPDATE query
    QpSqlQuery query(data->storage->database());
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    fillValuesIntoQuery(metaObject, object, query);

    query.addField(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG, Qp::Private::isDeleted(object));

#ifndef QP_NO_TIMESTAMPS
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif

    // Insert the object itself
    query.prepareUpdate();
    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    // Update related objects
    bool result = adjustRelationsInDatabase(metaObject, object);

#ifndef QP_NO_TIMESTAMPS
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, readUpdateTime(object));
#endif

    return result;
}

void QpSqlDataAccessObjectHelper::fillValuesIntoQuery(const QpMetaObject &metaObject,
                                                      const QObject *object,
                                                      QpSqlQuery &query)
{
    // Add simple properties
    foreach (const QpMetaProperty property, metaObject.simpleProperties()) {
        QVariant value = property.metaProperty().read(object);
        if(property.metaProperty().isEnumType() && value == QVariant(0)) {
            value = "NULL";
        }
        else {
            if(property.metaProperty().isEnumType() && !property.metaProperty().isFlagType())
#ifdef QP_FOR_MYSQL
                value = property.metaProperty().enumerator().valueToKey(value.toInt());
#elif QP_FOR_SQLITE
                value = value.toInt();
#endif

            if(property.metaProperty().isFlagType() && value == QVariant(0))
                value = "NULL";
        }

        query.addField(property.columnName(), value);
    }
}

void QpSqlDataAccessObjectHelper::selectFields(const QpMetaObject &metaObject, QpSqlQuery &query)
{

    foreach (const QpMetaProperty property, metaObject.simpleProperties()) {
        QString columnName = property.columnName();
#ifdef QP_FOR_MYSQL
        if(property.metaProperty().isEnumType())
            columnName += "+0";
#endif
        query.addField(columnName);
    }

    foreach(const QpMetaProperty relation, metaObject.relationProperties()) {
        if(relation.hasTableForeignKey()) {
            query.addField(relation.columnName());
        }
        else if(relation.isToOneRelationProperty()) {
            query.addRawField(QpSqlQuery::escapeField(relation.tableName()) + "." +
                              QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY +
                              " AS  _Qp_FK_" + relation.name());
            query.addJoin("LEFT", relation.tableName(),
                          QpSqlQuery::escapeField(relation.tableName()) + "." + QpSqlQuery::escapeField(relation.columnName()) +
                          " = " + query.escapedQualifiedField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY));
        }
    }

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
}

void QpSqlDataAccessObjectHelper::readQueryIntoObject(const QpSqlQuery &query,
                                                      const QSqlRecord record,
                                                      QObject *object,
                                                      int primaryKeyRecordIndex,
                                                      int updateTimeRecordIndex,
                                                      int deletedFlagRecordIndex)
{
#ifdef QP_NO_TIMESTAMPS
    Q_UNUSED(updateTimeRecordIndex);
#endif

    if(primaryKeyRecordIndex < 0)
        primaryKeyRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);

    int fieldCount = record.count();
    for (int i = 0; i < fieldCount; ++i) {

        QMetaProperty property = query.propertyForIndex(record, object->metaObject(), i);
        if(!property.isValid()) {
            object->setProperty(record.fieldName(i).toLatin1(), query.value(i));
            continue;
        }

        QVariant value = query.value(i);

        if(property.isFlagType()) {
            value = value.toInt();
        }
        else if(property.isEnumType()) {
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


    object->setProperty(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY, query.value(primaryKeyRecordIndex));

    if(deletedFlagRecordIndex < 0)
        deletedFlagRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG);

    bool deleted = query.value(deletedFlagRecordIndex).toBool();
    object->setProperty(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG, deleted);

#ifndef QP_NO_TIMESTAMPS
    if(updateTimeRecordIndex < 0)
        updateTimeRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME);

    object->setProperty(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, query.value(updateTimeRecordIndex));

    int creationTimeRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME);

    object->setProperty(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME, query.value(creationTimeRecordIndex));
#endif
#ifndef QP_NO_LOCKS
    int lockRecordIndex = record.indexOf(QpDatabaseSchema::COLUMN_LOCK);
    object->setProperty(QpDatabaseSchema::COLUMN_LOCK, query.value(lockRecordIndex));
#endif
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

    if(!relatedObject)
        return queries;

    QVariant relatedPrimary = Qp::Private::primaryKey(relatedObject.data());
    // Prepare actual update
    QpSqlQuery setForeignKeyQuery(data->storage->database());
    setForeignKeyQuery.setTable(relation.tableName());
    setForeignKeyQuery.addField(relation.columnName(), primaryKey);
#ifndef QP_NO_TIMESTAMPS
    setForeignKeyQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
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
    QpSqlQuery resetRelationQuery(data->storage->database());
    resetRelationQuery.setTable(relation.tableName());
    resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
    resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
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


#ifndef QP_NO_TIMESTAMPS
    QpSqlCondition relatedObjectsWhereClause2 = relatedObjectsWhereClause;
    relatedObjectsWhereClause2.setBindValuesAsString(true);
    QString updateTimeQueryString = QString("UPDATE %1 AS tableToUpdate"
                                            "\n\tINNER JOIN %2 "
                                            "\n\t\tON %2.%3 = tableToUpdate.%4 "
                                            "\n\tSET tableToUpdate.%5 = %6 "
                                            "\n\tWHERE %7")
            .arg(relation.metaObject().tableName())
            .arg(relation.tableName())
            .arg(relation.columnName())
            .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
            .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
            .arg(QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp())
            .arg(relatedObjectsWhereClause2.toWhereClause());

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
    if(selectPreviouslyRelatedObjectPKQuery.first())
        previousRelatedPK = selectPreviouslyRelatedObjectPKQuery.value(0);

    if(previousRelatedPK == relatedPrimary)
        return queries;

#ifndef QP_NO_TIMESTAMPS
    if(!previousRelatedPK.isNull()) {
        // Prepare a query, which adjusts the update time of a previously related object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQueryPreviouslyRelated(data->storage->database());
        adjustUpdateTimeQueryPreviouslyRelated.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQueryPreviouslyRelated.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
        adjustUpdateTimeQueryPreviouslyRelated.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                                QpSqlCondition::EqualTo,
                                                                                previousRelatedPK));
        adjustUpdateTimeQueryPreviouslyRelated.prepareUpdate();
        queries.append(adjustUpdateTimeQueryPreviouslyRelated);
    }
#endif

    if(!relatedPrimary.isNull()) {
        if(relation.cardinality() == QpMetaProperty::OneToOneCardinality) {
            // Prepare a query, which resets the relation (in my table: set old foreign key to NULL)
            QpSqlQuery resetRelationQuery(data->storage->database());
            resetRelationQuery.setTable(relation.tableName());
            resetRelationQuery.addField(relation.columnName(), QVariant());
#ifndef QP_NO_TIMESTAMPS
            resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
#endif
            resetRelationQuery.setWhereCondition(QpSqlCondition(relation.columnName(),
                                                                QpSqlCondition::EqualTo,
                                                                relatedPrimary));
            resetRelationQuery.prepareUpdate();
            queries.append(resetRelationQuery);
        }

#ifndef QP_NO_TIMESTAMPS
        // Prepare a query, which adjusts the update time of the new foreign object (in the foreign object's table)
        QpSqlQuery adjustUpdateTimeQuery(data->storage->database());
        adjustUpdateTimeQuery.setTable(relation.reverseMetaObject().tableName());
        adjustUpdateTimeQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp());
        adjustUpdateTimeQuery.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                               QpSqlCondition::EqualTo,
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

#ifndef QP_NO_TIMESTAMPS
    QpSqlCondition resetCondition2 = resetCondition;
    resetCondition2.setBindValuesAsString(true);
    // Update the times of now unrelated objects
    QString updatePreviouslyRelatedTimeQueryString = QString("UPDATE %1 AS tableToUpdate"
                                                             "\n\tINNER JOIN %2 "
                                                             "\n\t\tON %2.%3 = tableToUpdate.%4 "
                                                             "\n\tSET tableToUpdate.%5 = %6 "
                                                             "\n\tWHERE %7")
            .arg(QpSqlQuery::escapeField(relation.reverseRelation().metaObject().tableName()))
            .arg(relation.tableName())
            .arg(relation.reverseRelation().columnName())
            .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
            .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
            .arg(QpSqlBackend::forDatabase(data->storage->database())->nowTimestamp())
            .arg(resetCondition2.toWhereClause());

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
            .arg(relatedObjectsWhereClause2.toWhereClause())
            .arg(relation.tableName())
            .arg(relation.reverseRelation().columnName())
            .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
            .arg(relation.columnName())
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

bool QpSqlDataAccessObjectHelper::incrementNumericColumn(QObject *object, const QString &fieldName)
{
    static const int TRY_COUNT_MAX = 100;
    int tryCount = 0;
    do {
        ++tryCount;
        data->storage->database().transaction();
        QpMetaObject mo = QpMetaObject::forObject(object);
        QpSqlQuery query(data->storage->database());

        query.setTable(mo.tableName());
        query.addField(fieldName);
        query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                               QpSqlCondition::EqualTo,
                                               Qp::Private::primaryKey(object)));
        query.prepareincrementNumericColumn();

        if (!query.exec()
                || query.lastError().isValid()) {
            data->storage->database().rollback();

            if(tryCount < TRY_COUNT_MAX && query.lastError().nativeErrorCode() == QLatin1String("1213")) {
                qDebug() << Q_FUNC_INFO << QString::fromLatin1("MySQL found a deadlock. Re-trying transaction (try %1 of %2).")
                            .arg(tryCount)
                            .arg(TRY_COUNT_MAX);
                QThread::usleep(100);
                continue;
            }

            setLastError(query);
            return false;
        }

        data->storage->database().commit();
        return true;
    } while(true);
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
            || !query.first()
            || query.lastError().isValid()) {
        setLastError(query);
        return -1;
    }

    return query.value(0).toInt() - 1;
}

int QpSqlDataAccessObjectHelper::maxPrimaryKey(const QpMetaObject &metaObject) const
{
    QpSqlQuery query(data->storage->database());
    if (!query.exec(QString::fromLatin1(
                        "SELECT MAX(%1) "
                        "FROM  %2")
                    .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                    .arg(metaObject.tableName()))
            || !query.first()
            || query.lastError().isValid()) {
        setLastError(query);
        return -1;
    }

    return query.value(0).toInt() - 1;
}

#ifndef QP_NO_TIMESTAMPS
double QpSqlDataAccessObjectHelper::readUpdateTime(QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->storage->database());
    query.setTable(QpMetaObject::forObject(object).tableName());
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

double QpSqlDataAccessObjectHelper::readCreationTime(QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->storage->database());
    query.setTable(QpMetaObject::forObject(object).tableName());
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
#endif

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

    QpSqlQuery query(data->storage->database());
    query.setTable(relation.tableName());
    query.setWhereCondition(QpSqlCondition(keyColumn,
                                           QpSqlCondition::EqualTo,
                                           key));
    query.addField(foreignColumn);
    query.setForwardOnly(true);
    query.addOrder(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
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
    data->storage->setLastError(error);
}

void QpSqlDataAccessObjectHelper::setLastError(const QSqlQuery &query) const
{
    setLastError(QpError(QString("%1: %2")
                         .arg(query.lastError().text())
                         .arg(query.executedQuery()), QpError::SqlError));
}


