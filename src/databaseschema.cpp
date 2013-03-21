#include "databaseschema.h"

#include "sqlquery.h"
#include "metaproperty.h"
#include "error.h"
#include "metaobject.h"
#include "qpersistence.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMetaProperty>
#include <QStringList>
#include <QDebug>
#include <QSqlRecord>
#include <QFile>

class QPersistenceDatabaseSchemaPrivate : public QSharedData
{
public:
    QPersistenceDatabaseSchemaPrivate() :
        QSharedData()
    {}

    QSqlDatabase database;
    mutable QPersistenceError lastError;
    QPersistenceSqlQuery query;

    QString metaPropertyToColumnDefinition(const QPersistenceMetaProperty &property);
};

QPersistenceDatabaseSchema::QPersistenceDatabaseSchema(const QSqlDatabase &database, QObject *parent) :
    QObject(parent),
    d(new QPersistenceDatabaseSchemaPrivate)
{
    d->database = database;
    d->query = QPersistenceSqlQuery(database);
}

QPersistenceDatabaseSchema::~QPersistenceDatabaseSchema()
{
}

bool QPersistenceDatabaseSchema::existsTable(const QMetaObject &metaObject)
{
    QPersistenceMetaObject meta = QPersistence::metaObject(metaObject.className());
    return d->database.tables().contains(meta.tableName());
}

void QPersistenceDatabaseSchema::createTableIfNotExists(const QMetaObject &metaObject)
{
    if (!existsTable(metaObject))
        createTable(metaObject);
}

void QPersistenceDatabaseSchema::dropTable(const QMetaObject &metaObject)
{
    QPersistenceMetaObject meta = QPersistence::metaObject(metaObject.className());

    d->query.clear();
    d->query.setTable( meta.tableName());
    d->query.prepareDropTable();

    if ( !d->query.exec()
         || d->query.lastError().isValid()) {
        setLastError(d->query);
    }
}

void QPersistenceDatabaseSchema::createTable(const QMetaObject &metaObject)
{
    QPersistenceMetaObject meta = QPersistence::metaObject(metaObject.className());

    d->query.clear();
    d->query.setTable(meta.tableName());

    int count = metaObject.propertyCount();
    for (int i=1; i < count; ++i) { // start at 1 because 0 is "objectName"
        QPersistenceMetaProperty metaProperty(metaObject.property(i), meta);

        if (!metaProperty.isStored())
            continue;

        if(!metaProperty.isRelationProperty()) {
            QString columnName = metaProperty.columnName();
            QString columnType = QPersistenceDatabaseSchema::variantTypeToSqlType(metaProperty.type());

            if (metaProperty.isPrimaryKey()) {
                columnType.append(QLatin1String(" PRIMARY KEY"));
            }

            if(metaProperty.isAutoIncremented()) {
                columnType.append(QLatin1String(" AUTOINCREMENT"));
            }

            d->query.addField(columnName, columnType);
        }
        else {
            if(metaProperty.tableName() == meta.tableName()) {
                QPersistenceMetaProperty reverseRelation = metaProperty.reverseRelation();
                if(reverseRelation.isValid()) {
                    QPersistenceMetaObject reverseMetaObject = reverseRelation.metaObject();
                    QString columnName = metaProperty.columnName();
                    QString columnType = QPersistenceDatabaseSchema::variantTypeToSqlType(reverseMetaObject.primaryKeyProperty().type());

                    d->query.addField(columnName, columnType);
                    d->query.addForeignKey(metaProperty.columnName(),
                                           reverseMetaObject.primaryKeyProperty().columnName(),
                                           reverseMetaObject.tableName());
                }
            }
        }
    }

    d->query.prepareCreateTable();

    if ( !d->query.exec()
         || d->query.lastError().isValid()) {
        setLastError(d->query);
    }
}

bool QPersistenceDatabaseSchema::addMissingColumns(const QMetaObject &metaObject)
{
    QPersistenceMetaObject meta = QPersistence::metaObject(metaObject.className());
    QSqlRecord record = d->database.record(meta.tableName());;

    int count = metaObject.propertyCount();
    for (int i = 1; i < count; ++i) {
        QPersistenceMetaProperty metaProperty(metaObject.property(i), meta);

        if (!metaProperty.isStored())
            continue;

        if(metaProperty.isRelationProperty()) {
            QPersistenceMetaProperty::Cardinality cardinality = metaProperty.cardinality();
            if(cardinality == QPersistenceMetaProperty::ToManyCardinality
                    || cardinality == QPersistenceMetaProperty::OneToManyCardinality) {
                // The other table is responsible for adding this column
                continue;
            }
        }

        if (record.indexOf(metaProperty.columnName()) != -1)
            continue;

        addColumn(metaProperty);

        if (lastError().isValid())
            return false;
    }

    return true;
}

void QPersistenceDatabaseSchema::addColumn(const QPersistenceMetaProperty &metaProperty)
{
    QString tableName;
    QString name;
    QString type;

    if(metaProperty.isRelationProperty()) {
        name = metaProperty.columnName();
        tableName = metaProperty.tableName();
        type = QPersistenceDatabaseSchema::variantTypeToSqlType(metaProperty.foreignKeyType());

        qWarning("The relation %s will be added to the %s table. "
                 "But SQLite does not support adding foreign key contraints after a table has been created. "
                 "You might want to alter the table manually (by creating a new table and copying the original data).",
                 qPrintable(name),
                 qPrintable(tableName));
    }
    else {
        tableName = metaProperty.metaObject().tableName();
        name = metaProperty.columnName();
        type = QPersistenceDatabaseSchema::variantTypeToSqlType(metaProperty.type());
    }

    d->query.clear();
    d->query.setTable(tableName);
    d->query.addField(name, type);
    d->query.prepareAlterTable();

    if ( !d->query.exec()
         || d->query.lastError().isValid()) {
        setLastError(d->query);
    }
}

void QPersistenceDatabaseSchema::createCleanSchema()
{
    QFile file(d->database.databaseName());
    if(file.exists()) {
        if(!file.remove())
            qCritical() << Q_FUNC_INFO << "Could not remove database file"<< file.fileName();
        if(!d->database.open())
            qCritical() << Q_FUNC_INFO << "Could not re-open database file"<< file.fileName();
    }

    foreach(const QPersistenceMetaObject &metaObject, QPersistence::metaObjects()) {
        createTable(metaObject);
    }
}

void QPersistenceDatabaseSchema::adjustSchema()
{
    foreach(const QPersistenceMetaObject &metaObject, QPersistence::metaObjects()) {
        createTableIfNotExists(metaObject);
        addMissingColumns(metaObject);
    }
}

QPersistenceError QPersistenceDatabaseSchema::lastError() const
{
    return d->lastError;
}

QString QPersistenceDatabaseSchema::variantTypeToSqlType(QVariant::Type type)
{
    switch (type) {
    case QVariant::UInt:
    case QVariant::Int:
    case QVariant::Bool:
    case QVariant::ULongLong:
        return QLatin1String("INTEGER");
    case QVariant::String:
    case QVariant::StringList:
    case QVariant::Date:
    case QVariant::DateTime:
    case QVariant::Time:
    case QVariant::Char:
    case QVariant::Url:
        return QLatin1String("TEXT");
    case QVariant::Double:
        return QLatin1String("REAL");
    default:
        return QLatin1String("BLOB");
    }
}

void QPersistenceDatabaseSchema::setLastError(const QPersistenceError &error) const
{
    qDebug() << error;
    d->lastError = error;
}

void QPersistenceDatabaseSchema::setLastError(const QSqlQuery &query) const
{
    setLastError(QPersistenceError(query.lastError().text().append(": ").append(query.executedQuery()), QPersistenceError::SqlError));
}

QString QPersistenceDatabaseSchemaPrivate::metaPropertyToColumnDefinition(const QPersistenceMetaProperty &metaProperty)
{
    QString name;
    QString type;

    if(metaProperty.isRelationProperty()) {
        QPersistenceMetaProperty reverseRelation = metaProperty.reverseRelation();
        name = metaProperty.columnName();
        type = QPersistenceDatabaseSchema::variantTypeToSqlType(reverseRelation.metaObject().primaryKeyProperty().type());

        switch(metaProperty.cardinality()) {
        case QPersistenceMetaProperty::ToOneCardinality:
        case QPersistenceMetaProperty::OneToOneCardinality:
        case QPersistenceMetaProperty::ManyToOneCardinality:
            // My table gets a foreign key column
            break;
        case QPersistenceMetaProperty::ToManyCardinality:
        case QPersistenceMetaProperty::OneToManyCardinality:
            // The related table gets a foreign key column
            return QString();
        case QPersistenceMetaProperty::ManyToManyCardinality:
            // The relation need a whole table
            qDebug() << "Many to many relations are not supported yet.";
            return QString();
        default:
        case QPersistenceMetaProperty::NoCardinality:
            // This is BAD
            Q_ASSERT_X(false, Q_FUNC_INFO,
                       QString("The relation %1 has no cardinality. This is an internal error and should never happen.")
                       .arg(metaProperty.name())
                       .toLatin1());
            return QString();
        }
    }
    else {
        name = metaProperty.columnName();
        type = QPersistenceDatabaseSchema::variantTypeToSqlType(metaProperty.type());
    }

    if(name.isEmpty()
            || type.isEmpty()) {
        return QString();
    }

    return QString("\"%2\" %3")
            .arg(name)
            .arg(type);
}


