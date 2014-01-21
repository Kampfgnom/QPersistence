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

#include <QDebug>
#include <QMetaProperty>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QVariant>

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

QHash<QString, QpSqlDataAccessObjectHelper *> QpSqlDataAccessObjectHelperPrivate::helpersForConnection;

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
    static QObject guard;

    QpSqlDataAccessObjectHelper* asd = new QpSqlDataAccessObjectHelper(database, &guard);

    if (!QpSqlDataAccessObjectHelperPrivate::helpersForConnection.contains(database.connectionName()))
        QpSqlDataAccessObjectHelperPrivate::helpersForConnection.insert(database.connectionName(),
                                                                        asd);

    return QpSqlDataAccessObjectHelperPrivate::helpersForConnection.value(database.connectionName());
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

    readQueryIntoObject(query, object);
    return object;
}

QpSqlQuery QpSqlDataAccessObjectHelper::readAllObjects(const QpMetaObject &metaObject, int skip, int count)
{
    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
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
    fillValuesIntoQuery(metaObject, object, query, true);
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
    readDatabaseTimes(metaObject, object);

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

    // Update related objects
    return adjustRelationsInDatabase(metaObject, object);
}

void QpSqlDataAccessObjectHelper::fillValuesIntoQuery(const QpMetaObject &metaObject,
                                                      const QObject *object,
                                                      QpSqlQuery &query,
                                                      bool forInsert)
{
    // Add simple properties
    foreach (const QpMetaProperty property, metaObject.simpleProperties()) {
        query.addField(property.columnName(), property.metaProperty().read(object));
    }

    // There can not be any relations at insert time!
    if (!forInsert) {
        // Add relation properties
        foreach (const QpMetaProperty property, metaObject.relationProperties()) {
            QpMetaProperty::Cardinality cardinality = property.cardinality();

            // Only care for "XtoOne" relations, since only they have to be inserted into our table
            if (cardinality == QpMetaProperty::ToOneCardinality
                    || cardinality == QpMetaProperty::ManyToOneCardinality
                    || (QpMetaProperty::OneToOneCardinality
                        && property.hasTableForeignKey())) {
                QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(property.metaProperty().read(object));

                if (!relatedObject)
                    continue;

                QVariant foreignKey = Qp::Private::primaryKey(relatedObject.data());
                query.addField(property.columnName(), foreignKey);
            }
        }
    }
}

void QpSqlDataAccessObjectHelper::readQueryIntoObject(const QSqlQuery &query, QObject *object)
{
    QSqlRecord record = query.record();
    int fieldCount = record.count();
    for (int i = 0; i < fieldCount; ++i) {
        QByteArray fieldName = record.fieldName(i).toLatin1();
        QVariant value = query.value(i);

        int propertyIndex = object->metaObject()->indexOfProperty(fieldName);
        QMetaProperty property = object->metaObject()->property(propertyIndex);
        if(propertyIndex > 0 && property.isEnumType()) {
            value = value.toInt();
        }
        else {
            QMetaType::Type type = static_cast<QMetaType::Type>(property.userType());
            value = QpSqlQuery::variantFromSqlStorableVariant(value, type);
        }

        object->setProperty(fieldName, value);
    }
}

bool QpSqlDataAccessObjectHelper::adjustRelationsInDatabase(const QpMetaObject &metaObject, QObject *object)
{
    int primaryKey = Qp::Private::primaryKey(object);

    QList<QpSqlQuery> queries;

    QStringList updateTimeQueryStrings;

    foreach (const QpMetaProperty property, metaObject.relationProperties()) {
        QpMetaProperty::Cardinality cardinality = property.cardinality();

        // Only care for "XtoMany" relations, because these reside in other tables
        if (cardinality == QpMetaProperty::ToManyCardinality
                || cardinality == QpMetaProperty::OneToManyCardinality) {

            // Prepare a query, which resets the relation (set all foreign keys to NULL)
            QpSqlQuery resetRelationQuery(data->database);
            resetRelationQuery.setTable(property.tableName());
            resetRelationQuery.addField(property.columnName(), QVariant());
            resetRelationQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
            resetRelationQuery.setWhereCondition(QpSqlCondition(property.columnName(),
                                                                QpSqlCondition::EqualTo,
                                                                primaryKey));
            resetRelationQuery.prepareUpdate();
            queries.append(resetRelationQuery);

            // Check if there are related objects
            QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(property.metaProperty().read(object));
            if (relatedObjects.isEmpty())
                continue;

            // Build an OR'd where clause, which matches all related objects
            QList<QpSqlCondition> relatedObjectsWhereClauses;
            int count = 0;
            foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
                relatedObjectsWhereClauses.append(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                 QpSqlCondition::EqualTo,
                                                                 Qp::Private::primaryKey(relatedObject.data())));

                relatedObject->setProperty(property.columnName().toLatin1(), primaryKey);
                if (count > 990) {
                    QpSqlCondition relatedObjectsWhereClause(QpSqlCondition::Or, relatedObjectsWhereClauses);
                    QpSqlQuery setForeignKeysQuery(data->database);
                    setForeignKeysQuery.setTable(property.tableName());
                    setForeignKeysQuery.addField(property.columnName(), primaryKey);
                    setForeignKeysQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
                    setForeignKeysQuery.setWhereCondition(relatedObjectsWhereClause);
                    setForeignKeysQuery.prepareUpdate();
                    queries.append(setForeignKeysQuery);
                    relatedObjectsWhereClauses.clear();
                    count = 0;
                }

                ++count;
            }
            QpSqlCondition relatedObjectsWhereClause(QpSqlCondition::Or, relatedObjectsWhereClauses);

            // Prepare a query, which sets the foreign keys of the related objects to our objects key
            QpSqlQuery setForeignKeysQuery(data->database);
            setForeignKeysQuery.setTable(property.tableName());
            setForeignKeysQuery.addField(property.columnName(), primaryKey);
            setForeignKeysQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
            setForeignKeysQuery.setWhereCondition(relatedObjectsWhereClause);
            setForeignKeysQuery.prepareUpdate();
            queries.append(setForeignKeysQuery);
        }
        else if (cardinality == QpMetaProperty::OneToOneCardinality
                 && !property.hasTableForeignKey()) {
            QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(property.metaProperty().read(object));
            if (!relatedObject)
                continue;

            relatedObject->setProperty(property.columnName().toLatin1(), primaryKey);

            QVariant primary = Qp::Private::primaryKey(relatedObject.data());
            QVariant foreign = primaryKey;

            QpSqlQuery setForeignKeysQuery(data->database);
            setForeignKeysQuery.setTable(property.tableName());
            setForeignKeysQuery.addField(property.columnName(), primary);
            setForeignKeysQuery.addRawField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME, QpSqlBackend::forDatabase(data->database)->nowTimestamp());
            setForeignKeysQuery.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                                 QpSqlCondition::EqualTo,
                                                                 foreign));
            setForeignKeysQuery.prepareUpdate();
            queries.append(setForeignKeysQuery);
        }
        else if (cardinality == QpMetaProperty::ManyToManyCardinality) {
            // Prepare a query, which sets the update times of all related objects accordingly
            // This query will be executed twice: First for all previously related object, then for the new ones.
            // TODO: Make use of other SQL backends. This is MySQL only!
            QString updateTimeQueryString = QString("UPDATE %1"
                                                    "\n\tINNER JOIN %2 "
                                                    "\n\t\tON %2.%3 = %1.%4 "
                                                    "\n\tSET %1.%5 = %6 "
                                                    "\n\tWHERE %2.%7 = %8 ")
                    .arg(property.reverseRelation().metaObject().tableName())
                    .arg(property.tableName())
                    .arg(property.reverseRelation().columnName())
                    .arg(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY)
                    .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
                    .arg(QpSqlBackend::forDatabase(data->database)->nowTimestamp())
                    .arg(property.columnName())
                    .arg(primaryKey);

            updateTimeQueryStrings << updateTimeQueryString;
            QpSqlQuery setUpdateTimeOnRelatedObjectsQuery;
            setUpdateTimeOnRelatedObjectsQuery.prepare(updateTimeQueryString);
            queries.append(setUpdateTimeOnRelatedObjectsQuery);


            // Prepare a query, which resets the relation (deletes all relation rows)
            QpSqlQuery resetRelationQuery(data->database);
            resetRelationQuery.setTable(property.tableName());
            resetRelationQuery.setWhereCondition(QpSqlCondition(property.columnName(),
                                                                QpSqlCondition::EqualTo,
                                                                primaryKey));
            resetRelationQuery.prepareDelete();
            queries.append(resetRelationQuery);

            // Create rows, which represent the relation
            QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(property.metaProperty().read(object));
            if (relatedObjects.isEmpty())
                continue;

            foreach (QSharedPointer<QObject> relatedObject, relatedObjects) {
                QpSqlQuery createRelationQuery(data->database);
                createRelationQuery.setTable(property.tableName());
                createRelationQuery.addField(property.columnName(), primaryKey);
                createRelationQuery.addField(property.reverseRelation().columnName(), Qp::Private::primaryKey(relatedObject.data()));

                createRelationQuery.prepareInsert();
                queries.append(createRelationQuery);
            }
        }
    }

    foreach (QpSqlQuery query, queries) {
        if (!query.exec()
                || query.lastError().isValid()) {
            setLastError(query);
            return false;
        }
    }

    foreach(QString q, updateTimeQueryStrings) {
        QpSqlQuery query(data->database);
        if (!query.exec(q)
                || query.lastError().isValid()) {
            setLastError(query);
            return false;
        }
    }

    return true;
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

bool QpSqlDataAccessObjectHelper::readDatabaseTimes(const QpMetaObject &metaObject, QObject *object)
{
    Q_ASSERT(object);

    QpSqlQuery query(data->database);
    query.setTable(metaObject.tableName());
    query.setCount(1);
    query.addField(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME);
    query.addField(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareSelect();

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    if(!query.first())
        return false;

    readQueryIntoObject(query, object);
    return true;
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
        int key = query.value(0).toInt(&ok);
        if (ok)
            keys.append(key);
    }

    return keys;
}

void QpSqlDataAccessObjectHelper::setLastError(const QpError &error) const
{
    qWarning() << error;
    data->lastError = error;
    Qp::Private::setLastError(error);
}

void QpSqlDataAccessObjectHelper::setLastError(const QSqlQuery &query) const
{
    setLastError(QpError(QString("%1: %2")
                         .arg(query.lastError().text())
                         .arg(query.executedQuery()), QpError::SqlError));
}


