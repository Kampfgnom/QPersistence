#include "sqldataaccessobjecthelper.h"

#include "databaseschema.h"
#include "sqlquery.h"
#include "sqlcondition.h"
#include "dataaccessobject.h"
#include "metaproperty.h"
#include "error.h"
#include "metaobject.h"
#include "qpersistence.h"

#include <//qDebug>
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
    d(new QpSqlDataAccessObjectHelperPrivate)
{
    d->database = database;
}

QpSqlDataAccessObjectHelper::~QpSqlDataAccessObjectHelper()
{
}

QpSqlDataAccessObjectHelper *QpSqlDataAccessObjectHelper::forDatabase(const QSqlDatabase &database)
{
    static QObject guard;

    QpSqlDataAccessObjectHelper* asd = new QpSqlDataAccessObjectHelper(database, &guard);

    if(!QpSqlDataAccessObjectHelperPrivate::helpersForConnection.contains(database.connectionName()))
        QpSqlDataAccessObjectHelperPrivate::helpersForConnection.insert(database.connectionName(),
                                                                        asd);

    return QpSqlDataAccessObjectHelperPrivate::helpersForConnection.value(database.connectionName());
}

int QpSqlDataAccessObjectHelper::count(const QpMetaObject &metaObject) const
{
    QpSqlQuery query(d->database);
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

QList<int> QpSqlDataAccessObjectHelper::allKeys(const QpMetaObject &metaObject, int skip, int count) const
{
    //qDebug("\n\nallKeys<%s>", qPrintable(metaObject.tableName()));
    QpSqlQuery query(d->database);
    query.clear();
    query.setTable(metaObject.tableName());
    query.addField(QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME);
    query.setCount(count);
    query.setSkip(skip);
    query.prepareSelect();

    QString filter = metaObject.sqlFilter();
    if(!filter.isEmpty())
        query.setWhereCondition(QpSqlCondition(filter));

    QList<int> result;
    if ( !query.exec()
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
    //qDebug("\n\nreadObject<%s>(%s)", qPrintable(metaObject.tableName()), qPrintable(key.toString()));
    Q_ASSERT(object);
    Q_ASSERT(!key.isNull());

    QpSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    query.setCount(1);
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME,
                                           QpSqlCondition::EqualTo,
                                           key));
    query.prepareSelect();

    if ( !query.exec()
         || !query.first()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    readQueryIntoObject(query, object);
    return object;
}

bool QpSqlDataAccessObjectHelper::readAllObjects(const QpMetaObject &metaObject, QList<QObject *> objects, int skip, int count)
{
    Q_ASSERT(objects.size() == count);

    QpSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    query.setCount(count);
    query.setSkip(skip);
    query.prepareSelect();

    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    int i = 0;
    for(; i < objects.size() && query.next(); ++i) {
        readQueryIntoObject(query, objects.at(i));
    }
    Q_ASSERT(i == count);

    if (query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    return true;
}

bool QpSqlDataAccessObjectHelper::insertObject(const QpMetaObject &metaObject, QObject *object)
{
    //qDebug("\n\ninsertObject<%s>", qPrintable(metaObject.tableName()));
    Q_ASSERT(object);

    // Create main INSERT query
    QpSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    fillValuesIntoQuery(metaObject, object, query);

    // Insert the object itself
    query.prepareInsert();
    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    Qp::Private::setPrimaryKey(object, query.lastInsertId().toInt());

    return true;
}

bool QpSqlDataAccessObjectHelper::updateObject(const QpMetaObject &metaObject, QObject *object)
{
    //qDebug("\n\nupdateObject<%s>", qPrintable(metaObject.tableName()));
    Q_ASSERT(object);

    // Create main UPDATE query
    QpSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    fillValuesIntoQuery(metaObject, object, query);

    // Insert the object itself
    query.prepareUpdate();
    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    // Update related objects
    return adjustRelationsInDatabase(metaObject, object);
}

void QpSqlDataAccessObjectHelper::fillValuesIntoQuery(const QpMetaObject &metaObject,
                                                      const QObject *object,
                                                      QpSqlQuery &query)
{
    // Add simple properties
    foreach(const QpMetaProperty property, metaObject.simpleProperties()) {
        query.addField(property.columnName(), property.read(object));
    }

    // Add relation properties
    foreach(const QpMetaProperty property, metaObject.relationProperties()) {
        QpMetaProperty::Cardinality cardinality = property.cardinality();

        // Only care for "XtoOne" relations, since only they have to be inserted into our table
        if(cardinality == QpMetaProperty::ToOneCardinality
                || cardinality == QpMetaProperty::ManyToOneCardinality
                || (QpMetaProperty::OneToOneCardinality
                    && property.hasTableForeignKey())) {
            QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(property.read(object));

            if(!relatedObject)
                continue;

            QVariant foreignKey = Qp::Private::primaryKey(relatedObject.data());
            query.addField(property.columnName(), foreignKey);
        }
    }
}

void QpSqlDataAccessObjectHelper::readQueryIntoObject(const QSqlQuery &query, QObject *object)
{
    QSqlRecord record = query.record();
    int fieldCount = record.count();
    for (int i = 0; i < fieldCount; ++i) {
        QString fieldName = record.fieldName(i);
        QVariant value = query.value(i);

        value = QpSqlQuery::variantFromSqlStorableVariant(value, static_cast<QMetaType::Type>(object->property(fieldName.toLatin1()).userType()));
        object->setProperty(fieldName.toLatin1(), value);
    }
}

bool QpSqlDataAccessObjectHelper::adjustRelationsInDatabase(const QpMetaObject &metaObject, QObject *object)
{
    int primaryKey = Qp::Private::primaryKey(object);

    QList<QpSqlQuery> queries;

    foreach(const QpMetaProperty property, metaObject.relationProperties()) {
        QpMetaProperty::Cardinality cardinality = property.cardinality();

        // Only care for "XtoMany" relations, because these reside in other tables
        if(cardinality == QpMetaProperty::ToManyCardinality
                || cardinality == QpMetaProperty::OneToManyCardinality) {

            // Prepare a query, which resets the relation (set all foreign keys to NULL)
            QpSqlQuery resetRelationQuery(d->database);
            resetRelationQuery.setTable(property.tableName());
            resetRelationQuery.addField(property.columnName(), QVariant());
            resetRelationQuery.setWhereCondition(QpSqlCondition(property.columnName(),
                                                                QpSqlCondition::EqualTo,
                                                                primaryKey));
            resetRelationQuery.prepareUpdate();
            queries.append(resetRelationQuery);

            // Check if there are related objects
            QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(property.read(object));
            if(relatedObjects.isEmpty())
                continue;

            // Build an OR'd where clause, which matches all related objects
            QList<QpSqlCondition> relatedObjectsWhereClauses;
            int count = 0;
            foreach(QSharedPointer<QObject> relatedObject, relatedObjects) {
                relatedObjectsWhereClauses.append(QpSqlCondition(QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME,
                                                                 QpSqlCondition::EqualTo,
                                                                 Qp::Private::primaryKey(relatedObject.data())));

                relatedObject->setProperty(property.columnName().toLatin1(), primaryKey);
                if(count > 990) {
                    QpSqlCondition relatedObjectsWhereClause(QpSqlCondition::Or, relatedObjectsWhereClauses);
                    QpSqlQuery setForeignKeysQuery(d->database);
                    setForeignKeysQuery.setTable(property.tableName());
                    setForeignKeysQuery.addField(property.columnName(), primaryKey);
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
            QpSqlQuery setForeignKeysQuery(d->database);
            setForeignKeysQuery.setTable(property.tableName());
            setForeignKeysQuery.addField(property.columnName(), primaryKey);
            setForeignKeysQuery.setWhereCondition(relatedObjectsWhereClause);
            setForeignKeysQuery.prepareUpdate();
            queries.append(setForeignKeysQuery);
        }
        else if(cardinality == QpMetaProperty::OneToOneCardinality
                && !property.hasTableForeignKey()) {
            QSharedPointer<QObject> relatedObject = Qp::Private::objectCast(property.read(object));
            if(!relatedObject)
                continue;

            relatedObject->setProperty(property.columnName().toLatin1(), primaryKey);

            QVariant primary = Qp::Private::primaryKey(relatedObject.data());
            QVariant foreign = primaryKey;

            QpSqlQuery setForeignKeysQuery(d->database);
            setForeignKeysQuery.setTable(property.tableName());
            setForeignKeysQuery.addField(property.columnName(), primary);
            setForeignKeysQuery.setWhereCondition(QpSqlCondition(QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME,
                                                                 QpSqlCondition::EqualTo,
                                                                 foreign));
            setForeignKeysQuery.prepareUpdate();
            queries.append(setForeignKeysQuery);
        }
        else if(cardinality == QpMetaProperty::ManyToManyCardinality) {
            // Prepare a query, which resets the relation (deletes all relation rows)
            QpSqlQuery resetRelationQuery(d->database);
            resetRelationQuery.setTable(property.tableName());
            resetRelationQuery.setWhereCondition(QpSqlCondition(property.columnName(),
                                                                QpSqlCondition::EqualTo,
                                                                primaryKey));
            resetRelationQuery.prepareDelete();
            queries.append(resetRelationQuery);

            // Create rows, which represent the relation
            QList<QSharedPointer<QObject> > relatedObjects = Qp::Private::objectListCast(property.read(object));
            if(relatedObjects.isEmpty())
                continue;

            foreach(QSharedPointer<QObject> relatedObject, relatedObjects) {
                QpSqlQuery createRelationQuery(d->database);
                createRelationQuery.setTable(property.tableName());
                createRelationQuery.addField(property.columnName(), primaryKey);
                createRelationQuery.addField(property.reverseRelation().columnName(), Qp::Private::primaryKey(relatedObject.data()));

                createRelationQuery.prepareInsert();
                queries.append(createRelationQuery);
            }
        }
    }

    foreach(QpSqlQuery query, queries) {
        if ( !query.exec()
             || query.lastError().isValid()) {
            setLastError(query);
            return false;
        }
    }

    return true;
}

//bool QpSqlDataAccessObjectHelper::readRelatedObjects(const QpMetaObject &metaObject,
//                                                   QObject *object)
//{
//    // This static cache makes this method non-re-entrant!
//    // If we want some kind of thread safety someday, we have to do something about this
//    static QHash<QString, QHash<QVariant, QObject *> > alreadyReadObjectsPerTable;
//    static QObject *objectGraphRoot = 0;
//    if(!objectGraphRoot)
//        objectGraphRoot = object;

//    // Insert the current object into the cache
//    {
//        QHash<QVariant, QObject *> alreadyReadObjects = alreadyReadObjectsPerTable.value(metaObject.tableName());
//        alreadyReadObjects.insert(metaObject.primaryKey(object), object);
//        alreadyReadObjectsPerTable.insert(metaObject.tableName(), alreadyReadObjects);
//    }

//    foreach(const QpMetaProperty property, metaObject.relationProperties()) {
//        QpMetaProperty::Cardinality cardinality = property.cardinality();

//        QString className = property.reverseClassName();
//        QpDaoBase *dao = Qp::Private::dataAccessObject(property.reverseMetaObject());
//        if(!dao)
//            continue;

//        // Get the already read objects of the related table
//        QHash<QVariant, QObject *> alreadyReadRelatedObjects = alreadyReadObjectsPerTable.value(
//                    property.reverseMetaObject().className());

//        if(cardinality == QpMetaProperty::ToOneCardinality
//                || cardinality == QpMetaProperty::ManyToOneCardinality) {
//            QVariant foreignKey = object->property(property.columnName().toLatin1());
//            if(foreignKey.isNull())
//                continue;

//            QObject *relatedObject = 0;
//            if(alreadyReadRelatedObjects.contains(foreignKey)) {
//                relatedObject = alreadyReadRelatedObjects.value(foreignKey);
//            }
//            else {
//                relatedObject = dao->readObject(foreignKey);
//            }

//            QVariant value = Qp::Private::variantCast(relatedObject, className);

//            // Write the value even if it is NULL
//            object->setProperty(property.name(), value);
//        }
//        else if(cardinality == QpMetaProperty::ToManyCardinality
//                || cardinality == QpMetaProperty::OneToManyCardinality) {
//            // Construct a query, which selects all rows,
//            // which have our primary key as foreign
//            QpSqlQuery selectForeignKeysQuery(d->database);
//            selectForeignKeysQuery.setTable(property.tableName()); // select from foreign table
//            selectForeignKeysQuery.addField(property.reverseMetaObject().primaryKeyProperty().columnName());
//            selectForeignKeysQuery.setWhereCondition(QpSqlCondition(property.columnName(),
//                                                                  QpSqlCondition::EqualTo,
//                                                                  metaObject.primaryKey(object));
//            selectForeignKeysQuery.prepareSelect();

//            if ( !selectForeignKeysQuery.exec()
//                 || selectForeignKeysQuery.lastError().isValid()) {
//                setLastError(selectForeignKeysQuery);
//                return false;
//            }

//            QList<QObject *> relatedObjects;
//            while(selectForeignKeysQuery.next()) {
//                QVariant foreignKey = selectForeignKeysQuery.value(0);
//                QObject *relatedObject = 0;
//                if(alreadyReadRelatedObjects.contains(foreignKey)) {
//                    relatedObject = alreadyReadRelatedObjects.value(foreignKey);
//                }
//                else {
//                    relatedObject = dao->readObject(foreignKey);
//                }
//                relatedObjects.append(relatedObject);
//            }
//            QVariant value = Qp::Private::variantListCast(relatedObjects, className);
//            object->setProperty(property.name(), value);
//        }
//        else if(cardinality == QpMetaProperty::ManyToManyCardinality) {
//            Q_ASSERT_X(false, Q_FUNC_INFO, "ManyToManyCardinality relations are not supported yet.");
//        }
//        else if(cardinality == QpMetaProperty::OneToOneCardinality) {
//            Q_ASSERT_X(false, Q_FUNC_INFO, "OneToOneCardinality relations are not supported yet.");
//        }
//    }

//    // clear the caches
//    if(object == objectGraphRoot) {
//        alreadyReadObjectsPerTable.clear();
//        objectGraphRoot = 0;
//    }

//    return true;
//}

bool QpSqlDataAccessObjectHelper::removeObject(const QpMetaObject &metaObject, QObject *object)
{
    //qDebug("\n\nremoveObject<%s>", qPrintable(metaObject.tableName()));
    Q_ASSERT(object);

    QpSqlQuery query(d->database);
    query.setTable(metaObject.tableName());
    query.setWhereCondition(QpSqlCondition(QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME,
                                           QpSqlCondition::EqualTo,
                                           Qp::Private::primaryKey(object)));
    query.prepareDelete();

    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    return true;
}

QpError QpSqlDataAccessObjectHelper::lastError() const
{
    return d->lastError;
}

int QpSqlDataAccessObjectHelper::foreignKey(const QpMetaProperty relation, QObject *object)
{
    QList<int> keys = foreignKeys(relation, object);
    if(keys.isEmpty())
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

    if(cardinality == QpMetaProperty::OneToManyCardinality
            || cardinality == QpMetaProperty::OneToOneCardinality) {
        keyColumn = relation.reverseRelation().columnName();
        foreignColumn = QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME;

        if(relation.hasTableForeignKey()) {
            qSwap(keyColumn, foreignColumn);
        }

        sortColumn = foreignColumn;
    }
    else if(cardinality == QpMetaProperty::ManyToOneCardinality) {
        keyColumn = QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME;
        foreignColumn = relation.reverseRelation().columnName();
    }
    else if(cardinality == QpMetaProperty::ManyToManyCardinality) {
        keyColumn = relation.columnName();
        foreignColumn = relation.reverseRelation().columnName();
        sortColumn = QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME;
    }

    Q_ASSERT(!foreignColumn.isEmpty());
    Q_ASSERT(!keyColumn.isEmpty());

    QpSqlQuery query(d->database);
    query.setTable(relation.tableName());
    query.setWhereCondition(QpSqlCondition(keyColumn,
                                           QpSqlCondition::EqualTo,
                                           key));
    query.addField(foreignColumn);
    if(!sortColumn.isEmpty())
        query.addOrder(sortColumn);
    query.prepareSelect();

    if ( !query.exec()
         || query.lastError().isValid()) {
        setLastError(query);
        return QList<int>();
    }

    bool ok = true;
    QList<int> keys;
    keys.reserve(query.size());
    while(query.next()) {
        int key = query.value(0).toInt(&ok);
        if(ok)
            keys.append(key);
    }

    return keys;
}

void QpSqlDataAccessObjectHelper::setLastError(const QpError &error) const
{
    qDebug() << error;
    qFatal("Aborting due to SQL errors");
    d->lastError = error;
}

void QpSqlDataAccessObjectHelper::setLastError(const QSqlQuery &query) const
{
    setLastError(QpError(query.lastError().text().append(": ").append(query.executedQuery()), QpError::SqlError));
}


