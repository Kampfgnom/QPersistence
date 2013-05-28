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

const QString QpDatabaseSchema::PRIMARY_KEY_COLUMN_NAME("_Qp_ID");

class QpDatabaseSchemaPrivate : public QSharedData
{
public:
    QpDatabaseSchemaPrivate() :
        QSharedData()
    {}

    QSqlDatabase database;
    mutable QpError lastError;
    QpSqlQuery query;

    QString metaPropertyToColumnDefinition(const QpMetaProperty &property);
};

QpDatabaseSchema::QpDatabaseSchema(const QSqlDatabase &database, QObject *parent) :
    QObject(parent),
    d(new QpDatabaseSchemaPrivate)
{
    d->database = database;
    d->query = QpSqlQuery(database);
}

QpDatabaseSchema::~QpDatabaseSchema()
{
}

bool QpDatabaseSchema::existsTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = Qp::Private::metaObject(metaObject.className());
    return d->database.tables().contains(meta.tableName());
}

void QpDatabaseSchema::createTableIfNotExists(const QMetaObject &metaObject)
{
    if (!existsTable(metaObject))
        createTable(metaObject);
}

void QpDatabaseSchema::dropTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = Qp::Private::metaObject(metaObject.className());

    d->query.clear();
    d->query.setTable( meta.tableName());
    d->query.prepareDropTable();

    if ( !d->query.exec()
         || d->query.lastError().isValid()) {
        setLastError(d->query);
    }
}

void QpDatabaseSchema::createTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = Qp::Private::metaObject(metaObject.className());

    d->query.clear();
    d->query.setTable(meta.tableName());

    int count = metaObject.propertyCount();
    for (int i=1; i < count; ++i) { // start at 1 because 0 is "objectName"
        QpMetaProperty metaProperty(metaObject.property(i), meta);

        if (!metaProperty.isStored())
            continue;

        if(metaProperty.isRelationProperty()) {
            if(metaProperty.tableName() == meta.tableName()) {
                QpMetaProperty reverseRelation = metaProperty.reverseRelation();
                if(reverseRelation.isValid()) {
                    QpMetaObject reverseMetaObject = reverseRelation.metaObject();
                    QString columnName = metaProperty.columnName();
                    QString columnType = QpDatabaseSchema::variantTypeToSqlType(QVariant::Int);

                    d->query.addField(columnName, columnType);
                    d->query.addForeignKey(metaProperty.columnName(),
                                           PRIMARY_KEY_COLUMN_NAME,
                                           reverseMetaObject.tableName());
                }
            }
        }
        else {
            QString columnName = metaProperty.columnName();
            QString columnType = QpDatabaseSchema::variantTypeToSqlType(metaProperty.type());

            d->query.addField(columnName, columnType);
        }
    }

    // Add the primary key
    d->query.addField(PRIMARY_KEY_COLUMN_NAME, "INTEGER PRIMARY KEY AUTOINCREMENT");

    d->query.prepareCreateTable();

    if ( !d->query.exec()
         || d->query.lastError().isValid()) {
        setLastError(d->query);
        return;
    }

    createRelationTables(metaObject);
}

void QpDatabaseSchema::createRelationTables(const QMetaObject &metaObject)
{
    QpMetaObject meta = Qp::Private::metaObject(metaObject.className());
    QString primaryTable = meta.tableName();
    QString columnType = QpDatabaseSchema::variantTypeToSqlType(QVariant::Int);

    foreach(QpMetaProperty property, meta.relationProperties()) {
        if(property.cardinality() != QpMetaProperty::ManyToManyCardinality)
            continue;

        QString tableName = property.tableName();
        if(d->database.tables().contains(tableName))
            continue;

        QString columnName = property.columnName();
        QString foreignTable = property.reverseMetaObject().tableName();
        QString foreignColumnName = property.reverseRelation().columnName();

        QpSqlQuery createTableQuery(d->database);
        createTableQuery.setTable(tableName);
        createTableQuery.addField(columnName, columnType);
        createTableQuery.addField(foreignColumnName, columnType);
        createTableQuery.addForeignKey(columnName,
                                       PRIMARY_KEY_COLUMN_NAME,
                                       primaryTable);
        createTableQuery.addForeignKey(foreignColumnName,
                                       PRIMARY_KEY_COLUMN_NAME,
                                       foreignTable);
        createTableQuery.prepareCreateTable();
        if ( !createTableQuery.exec()
             || createTableQuery.lastError().isValid()) {
            setLastError(createTableQuery);
            return;
        }
    }
}


bool QpDatabaseSchema::addMissingColumns(const QMetaObject &metaObject)
{
    QpMetaObject meta = Qp::Private::metaObject(metaObject.className());
    QSqlRecord record = d->database.record(meta.tableName());;

    int count = metaObject.propertyCount();
    for (int i = 1; i < count; ++i) {
        QpMetaProperty metaProperty(metaObject.property(i), meta);

        if (!metaProperty.isStored())
            continue;

        if(metaProperty.isRelationProperty()) {
            QpMetaProperty::Cardinality cardinality = metaProperty.cardinality();
            if(cardinality == QpMetaProperty::ToManyCardinality
                    || cardinality == QpMetaProperty::OneToManyCardinality) {
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

void QpDatabaseSchema::addColumn(const QpMetaProperty &metaProperty)
{
    QString tableName;
    QString name;
    QString type;

    if(metaProperty.isRelationProperty()) {
        name = metaProperty.columnName();
        tableName = metaProperty.tableName();
        type = QpDatabaseSchema::variantTypeToSqlType(QVariant::Int);

        qWarning("The relation %s will be added to the %s table. "
                 "But SQLite does not support adding foreign key contraints after a table has been created. "
                 "You might want to alter the table manually (by creating a new table and copying the original data).",
                 qPrintable(name),
                 qPrintable(tableName));
    }
    else {
        tableName = metaProperty.metaObject().tableName();
        name = metaProperty.columnName();
        type = QpDatabaseSchema::variantTypeToSqlType(metaProperty.type());
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

void QpDatabaseSchema::createCleanSchema()
{
    QFile file(d->database.databaseName());
    if(file.exists()) {
        if(!file.remove())
            qCritical() << Q_FUNC_INFO << "Could not remove database file"<< file.fileName();
        if(!d->database.open())
            qCritical() << Q_FUNC_INFO << "Could not re-open database file"<< file.fileName();
    }

    foreach(const QpMetaObject &metaObject, Qp::Private::metaObjects()) {
        createTable(metaObject);
    }
}

void QpDatabaseSchema::adjustSchema()
{
    foreach(const QpMetaObject &metaObject, Qp::Private::metaObjects()) {
        createTableIfNotExists(metaObject);
        addMissingColumns(metaObject);
    }
}

QpError QpDatabaseSchema::lastError() const
{
    return d->lastError;
}

QString QpDatabaseSchema::variantTypeToSqlType(QVariant::Type type)
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
    case QVariant::UserType:
    default:
        return QLatin1String("BLOB");
    }
}

void QpDatabaseSchema::setLastError(const QpError &error) const
{
    qDebug() << error;
    d->lastError = error;
}

void QpDatabaseSchema::setLastError(const QSqlQuery &query) const
{
    setLastError(QpError(query.lastError().text().append(": ").append(query.executedQuery()), QpError::SqlError));
}
QString QpDatabaseSchemaPrivate::metaPropertyToColumnDefinition(const QpMetaProperty &metaProperty)
{
    QString name;
    QString type;

    if(metaProperty.isRelationProperty()) {
        name = metaProperty.columnName();
        type = QpDatabaseSchema::variantTypeToSqlType(QVariant::Int);

        switch(metaProperty.cardinality()) {
        case QpMetaProperty::ToOneCardinality:
        case QpMetaProperty::OneToOneCardinality:
        case QpMetaProperty::ManyToOneCardinality:
            // My table gets a foreign key column
            break;
        case QpMetaProperty::ToManyCardinality:
        case QpMetaProperty::OneToManyCardinality:
            // The related table gets a foreign key column
            return QString();
        case QpMetaProperty::ManyToManyCardinality:
            // The relation need a whole table
            break;

        default:
        case QpMetaProperty::NoCardinality:
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
        type = QpDatabaseSchema::variantTypeToSqlType(metaProperty.type());
    }

    if(name.isEmpty()
            || type.isEmpty()) {
        return QString();
    }

    return QString("\"%2\" %3")
            .arg(name)
            .arg(type);
}


