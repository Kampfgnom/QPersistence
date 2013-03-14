#include "sqldataaccessobjecthelper.h"

#include "databaseschema.h"
#include "sqlquery.h"
#include "sqlcondition.h"
#include "persistentdataaccessobject.h"
#include "metaproperty.h"
#include "error.h"
#include "metaobject.h"
#include "qpersistence.h"

#include <QDebug>
#include <QMetaProperty>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QVariant>



class QPersistenceSqlDataAccessObjectHelperPrivate : public QSharedData
{
public:
    QPersistenceSqlDataAccessObjectHelperPrivate() :
        QSharedData()
    {}

    QSqlDatabase database;
    mutable QPersistenceError lastError;

    static QHash<QString, QPersistenceSqlDataAccessObjectHelper *> helpersForConnection;
};

QHash<QString, QPersistenceSqlDataAccessObjectHelper *> QPersistenceSqlDataAccessObjectHelperPrivate::helpersForConnection;

QPersistenceSqlDataAccessObjectHelper::QPersistenceSqlDataAccessObjectHelper(const QSqlDatabase &database, QObject *parent) :
    QObject(parent),
    d(new QPersistenceSqlDataAccessObjectHelperPrivate)
{
    d->database = database;
    QPersistenceSqlQuery query(database);
    query.prepare("PRAGMA foreign_keys = 1;");
    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
    }
}

QPersistenceSqlDataAccessObjectHelper::~QPersistenceSqlDataAccessObjectHelper()
{
}

QPersistenceSqlDataAccessObjectHelper *QPersistenceSqlDataAccessObjectHelper::forDatabase(const QSqlDatabase &database)
{
    static QObject guard;

    QPersistenceSqlDataAccessObjectHelper* asd = new QPersistenceSqlDataAccessObjectHelper(database, &guard);

    if(!QPersistenceSqlDataAccessObjectHelperPrivate::helpersForConnection.contains(database.connectionName()))
        QPersistenceSqlDataAccessObjectHelperPrivate::helpersForConnection.insert(database.connectionName(),
                                                                      asd);

    return QPersistenceSqlDataAccessObjectHelperPrivate::helpersForConnection.value(database.connectionName());
}

int QPersistenceSqlDataAccessObjectHelper::count(const QPersistenceMetaObject &metaObject) const
{
    QPersistenceSqlQuery query(d->database);
    query.prepare(QString("SELECT COUNT(*) FROM %1")
                  .arg(metaObject.tableName()));

    if ( !query.exec()
         || !query.first()
         || query.lastError().isValid()) {
        setLastError(query);
        return 0;
    }

    return query.value(0).toInt();
}

QList<QVariant> QPersistenceSqlDataAccessObjectHelper::allKeys(const QPersistenceMetaObject &metaObject) const
{
    qDebug("\n\nallKeys<%s>", qPrintable(metaObject.tableName()));
    QPersistenceSqlQuery query(d->database);
    query.clear();
    query.setTable(metaObject.tableName());
    query.addField(metaObject.primaryKeyPropertyName());
    query.prepareSelect();

    QList<QVariant> result;
    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return result;
    }

    while (query.next()) {
        result.append(query.value(0));
    }

    return result;
}

bool QPersistenceSqlDataAccessObjectHelper::readObject(const QPersistenceMetaObject &metaObject,
                                           const QVariant &key,
                                           QObject *object)
{
    qDebug("\n\nreadObject<%s>(%s)", qPrintable(metaObject.tableName()), qPrintable(key.toString()));
    Q_ASSERT(object);
    Q_ASSERT(!key.isNull());

    QPersistenceSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    query.setLimit(1);
    query.setWhereCondition(QPersistenceSqlCondition(metaObject.primaryKeyProperty().columnName(),
                                         QPersistenceSqlCondition::EqualTo,
                                         key));
    query.prepareSelect();

    if ( !query.exec()
         || !query.first()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    readQueryIntoObject(query, object);
    return readRelatedObjects(metaObject, object);
}

bool QPersistenceSqlDataAccessObjectHelper::insertObject(const QPersistenceMetaObject &metaObject, QObject *object)
{
    qDebug("\n\ninsertObject<%s>", qPrintable(metaObject.tableName()));
    Q_ASSERT(object);

    // Create main INSERT query
    QPersistenceSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    fillValuesIntoQuery(metaObject, object, query);

    // Insert the object itself
    query.prepareInsert();
    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    if(metaObject.primaryKeyProperty().isAutoIncremented()) {
        metaObject.primaryKeyProperty().write(object, query.lastInsertId());
    }

    // Update related objects
    return adjustRelations(metaObject, object);
}

bool QPersistenceSqlDataAccessObjectHelper::updateObject(const QPersistenceMetaObject &metaObject, const QObject *object)
{
    qDebug("\n\nupdateObject<%s>", qPrintable(metaObject.tableName()));
    Q_ASSERT(object);

    // Create main UPDATE query
    QPersistenceSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QPersistenceSqlCondition(metaObject.primaryKeyProperty().columnName(),
                                         QPersistenceSqlCondition::EqualTo,
                                         metaObject.primaryKeyProperty().read(object)));
    fillValuesIntoQuery(metaObject, object, query);

    // Insert the object itself
    query.prepareUpdate();
    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    // Update related objects
    return adjustRelations(metaObject, object);
}

void QPersistenceSqlDataAccessObjectHelper::fillValuesIntoQuery(const QPersistenceMetaObject &metaObject,
                                                    const QObject *object,
                                                    QPersistenceSqlQuery &query)
{
    // Add simple properties
    foreach(const QPersistenceMetaProperty property, metaObject.simpleProperties()) {
        if(!property.isAutoIncremented()) {
            query.addField(property.columnName(), property.read(object));
        }
    }

    // Add relation properties
    foreach(const QPersistenceMetaProperty property, metaObject.relationProperties()) {
        QPersistenceMetaProperty::Cardinality cardinality = property.cardinality();

        // Only care for "XtoOne" relations, since only they have to be inserted into our table
        if(cardinality == QPersistenceMetaProperty::ToOneCardinality
                || cardinality == QPersistenceMetaProperty::ManyToOneCardinality) {
            QObject *relatedObject = QPersistence::Private::objectCast(property.read(object));

            if(!relatedObject)
                continue;

            QVariant foreignKey = property.reverseMetaObject().primaryKeyProperty().read(relatedObject);
            query.addField(property.columnName(), foreignKey);
        }
        else if(cardinality == QPersistenceMetaProperty::OneToOneCardinality) {
            Q_ASSERT_X(false, Q_FUNC_INFO, "OneToOneCardinality relations are not supported yet.");
        }
    }
}

void QPersistenceSqlDataAccessObjectHelper::readQueryIntoObject(const QSqlQuery &query, QObject *object)
{
    QSqlRecord record = query.record();
    int fieldCount = record.count();
    for (int i = 0; i < fieldCount; ++i) {
        QString fieldName = record.fieldName(i);
        QVariant value = query.value(i);
        object->setProperty(fieldName.toLatin1(), value);
    }
}

bool QPersistenceSqlDataAccessObjectHelper::adjustRelations(const QPersistenceMetaObject &metaObject, const QObject *object)
{
    QVariant primaryKey = metaObject.primaryKeyProperty().read(object);

    QList<QPersistenceSqlQuery> queries;

    foreach(const QPersistenceMetaProperty property, metaObject.relationProperties()) {
        QPersistenceMetaProperty::Cardinality cardinality = property.cardinality();

        // Only care for "XtoMany" relations, because these reside in other tables
        if(cardinality == QPersistenceMetaProperty::ToManyCardinality
                || cardinality == QPersistenceMetaProperty::OneToManyCardinality) {
            QPersistenceMetaProperty reversePrimaryKey = property.reverseMetaObject().primaryKeyProperty();

            // Prepare a query, which resets the relation (set all foreign keys to NULL)
            QPersistenceSqlQuery resetRelationQuery(d->database);
            resetRelationQuery.setTable(property.tableName());
            resetRelationQuery.addField(property.columnName(), QVariant());
            resetRelationQuery.setWhereCondition(QPersistenceSqlCondition(property.columnName(),
                                                              QPersistenceSqlCondition::EqualTo,
                                                              primaryKey));
            resetRelationQuery.prepareUpdate();
            queries.append(resetRelationQuery);

            // Check if there are related objects
            QList<QObject *> relatedObjects = QPersistence::Private::objectListCast(property.read(object));
            if(relatedObjects.isEmpty())
                continue;

            // Build an OR'd where clause, which matches all related objects
            QList<QPersistenceSqlCondition> relatedObjectsWhereClauses;
            foreach(QObject *relatedObject, relatedObjects) {
                relatedObjectsWhereClauses.append(QPersistenceSqlCondition(reversePrimaryKey.columnName(),
                                                               QPersistenceSqlCondition::EqualTo,
                                                               reversePrimaryKey.read(relatedObject)));
            }
            QPersistenceSqlCondition relatedObjectsWhereClause(QPersistenceSqlCondition::Or, relatedObjectsWhereClauses);

            // Prepare a query, which sets the foreign keys of the related objects to our objects key
            QPersistenceSqlQuery setForeignKeysQuery(d->database);
            setForeignKeysQuery.setTable(property.tableName());
            setForeignKeysQuery.addField(property.columnName(), primaryKey);
            setForeignKeysQuery.setWhereCondition(relatedObjectsWhereClause);
            setForeignKeysQuery.prepareUpdate();
            queries.append(setForeignKeysQuery);
        }
        else if(cardinality == QPersistenceMetaProperty::ManyToManyCardinality) {
            Q_ASSERT_X(false, Q_FUNC_INFO, "ManyToManyCardinality relations are not supported yet.");
        }
        else if(cardinality == QPersistenceMetaProperty::OneToOneCardinality) {
            Q_ASSERT_X(false, Q_FUNC_INFO, "OneToOneCardinality relations are not supported yet.");
        }
    }

    foreach(QPersistenceSqlQuery query, queries) {
        if ( !query.exec()
             || query.lastError().isValid()) {
            setLastError(query);
            return false;
        }
    }

    return true;
}

bool QPersistenceSqlDataAccessObjectHelper::readRelatedObjects(const QPersistenceMetaObject &metaObject,
                                                   QObject *object)
{
    // This static cache makes this method non-re-entrant!
    // If we want some kind of thread safety someday, we have to do something about this
    static QHash<QString, QHash<QVariant, QObject *> > alreadyReadObjectsPerTable;
    static QObject *objectGraphRoot = 0;
    if(!objectGraphRoot)
        objectGraphRoot = object;

    // Insert the current object into the cache
    {
        QHash<QVariant, QObject *> alreadyReadObjects = alreadyReadObjectsPerTable.value(metaObject.tableName());
        alreadyReadObjects.insert(metaObject.primaryKeyProperty().read(object), object);
        alreadyReadObjectsPerTable.insert(metaObject.tableName(), alreadyReadObjects);
    }

    foreach(const QPersistenceMetaProperty property, metaObject.relationProperties()) {
        QPersistenceMetaProperty::Cardinality cardinality = property.cardinality();

        QString className = property.reverseClassName();
        QPersistenceAbstractDataAccessObject *dao = QPersistence::dataAccessObject(property.reverseMetaObject(), d->database.connectionName());
        if(!dao)
            continue;

        // Get the already read objects of the related table
        QHash<QVariant, QObject *> alreadyReadRelatedObjects = alreadyReadObjectsPerTable.value(
                    property.reverseMetaObject().className());

        if(cardinality == QPersistenceMetaProperty::ToOneCardinality
                || cardinality == QPersistenceMetaProperty::ManyToOneCardinality) {
            QVariant foreignKey = object->property(property.columnName().toLatin1());
            if(foreignKey.isNull())
                continue;

            QObject *relatedObject = 0;
            if(alreadyReadRelatedObjects.contains(foreignKey)) {
                relatedObject = alreadyReadRelatedObjects.value(foreignKey);
            }
            else {
                relatedObject = dao->readObject(foreignKey);
            }

            QVariant value = QPersistence::Private::variantCast(relatedObject, className);

            // Write the value even if it is NULL
            object->setProperty(property.name(), value);
        }
        else if(cardinality == QPersistenceMetaProperty::ToManyCardinality
                || cardinality == QPersistenceMetaProperty::OneToManyCardinality) {
            // Construct a query, which selects all rows,
            // which have our primary key as foreign
            QPersistenceSqlQuery selectForeignKeysQuery(d->database);
            selectForeignKeysQuery.setTable(property.tableName()); // select from foreign table
            selectForeignKeysQuery.addField(property.reverseMetaObject().primaryKeyProperty().columnName());
            selectForeignKeysQuery.setWhereCondition(QPersistenceSqlCondition(property.columnName(),
                                                                  QPersistenceSqlCondition::EqualTo,
                                                                  metaObject.primaryKeyProperty().read(object)));
            selectForeignKeysQuery.prepareSelect();

            if ( !selectForeignKeysQuery.exec()
                 || selectForeignKeysQuery.lastError().isValid()) {
                setLastError(selectForeignKeysQuery);
                return false;
            }

            QList<QObject *> relatedObjects;
            while(selectForeignKeysQuery.next()) {
                QVariant foreignKey = selectForeignKeysQuery.value(0);
                QObject *relatedObject = 0;
                if(alreadyReadRelatedObjects.contains(foreignKey)) {
                    relatedObject = alreadyReadRelatedObjects.value(foreignKey);
                }
                else {
                    relatedObject = dao->readObject(foreignKey);
                }
                relatedObjects.append(relatedObject);
            }
            QVariant value = QPersistence::Private::variantListCast(relatedObjects, className);
            object->setProperty(property.name(), value);
        }
        else if(cardinality == QPersistenceMetaProperty::ManyToManyCardinality) {
            Q_ASSERT_X(false, Q_FUNC_INFO, "ManyToManyCardinality relations are not supported yet.");
        }
        else if(cardinality == QPersistenceMetaProperty::OneToOneCardinality) {
            Q_ASSERT_X(false, Q_FUNC_INFO, "OneToOneCardinality relations are not supported yet.");
        }
    }

    // clear the caches
    if(object == objectGraphRoot) {
        alreadyReadObjectsPerTable.clear();
        objectGraphRoot = 0;
    }

    return true;
}

bool QPersistenceSqlDataAccessObjectHelper::removeObject(const QPersistenceMetaObject &metaObject, const QObject *object)
{
    qDebug("\n\nremoveObject<%s>", qPrintable(metaObject.tableName()));
    Q_ASSERT(object);

    QPersistenceSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QPersistenceSqlCondition(metaObject.primaryKeyProperty().columnName(),
                                         QPersistenceSqlCondition::EqualTo,
                                         metaObject.primaryKeyProperty().read(object)));
    query.prepareDelete();

    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    return true;
}

QPersistenceError QPersistenceSqlDataAccessObjectHelper::lastError() const
{
    return d->lastError;
}

void QPersistenceSqlDataAccessObjectHelper::setLastError(const QPersistenceError &error) const
{
    qDebug() << error;
    d->lastError = error;
}

void QPersistenceSqlDataAccessObjectHelper::setLastError(const QSqlQuery &query) const
{
    setLastError(QPersistenceError(query.lastError().text().append(": ").append(query.executedQuery()), QPersistenceError::SqlError));
}


