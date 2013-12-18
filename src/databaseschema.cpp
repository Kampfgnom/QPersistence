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
#include <QSqlField>

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
    return existsTable(meta.tableName());
}

bool QpDatabaseSchema::existsTable(const QString &table)
{
    return d->database.tables().contains(table);
}


void QpDatabaseSchema::createTableIfNotExists(const QMetaObject &metaObject)
{
    if (!existsTable(metaObject))
        createTable(metaObject);
}

void QpDatabaseSchema::dropTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = Qp::Private::metaObject(metaObject.className());

    dropTable(meta.tableName());
}

void QpDatabaseSchema::dropTable(const QString &table)
{
    d->query.clear();
    d->query.setTable(table);
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
    }
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
        createTableQuery.addField(PRIMARY_KEY_COLUMN_NAME, "INTEGER PRIMARY KEY AUTOINCREMENT");
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

    int count = metaObject.propertyCount();
    for (int i = 1; i < count; ++i) {
        QpMetaProperty metaProperty(metaObject.property(i), meta);
        QSqlRecord record = d->database.record(metaProperty.tableName());;

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

    addColumn(tableName, name, type);
}

void QpDatabaseSchema::addColumn(const QString &table, const QString &column, const QString &type)
{

    d->query.clear();
    d->query.setTable(table);
    d->query.addField(column, type);
    d->query.prepareAlterTable();

    if ( !d->query.exec()
         || d->query.lastError().isValid()) {
        setLastError(d->query);
    }
}

bool QpDatabaseSchema::dropColumns(const QString &table, const QStringList &columns)
{
    d->database.close();
    d->database.open();

    if(!d->database.transaction())
        return false;

    QpSqlQuery query(d->database);
    query.exec(QString("SELECT sql FROM sqlite_master WHERE name = '%1'")
               .arg(table));
    if (!query.first()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    QString sql = query.value(0).toString();

    QString tableBackup = QString("_Qp_BACKUP_").append(table);
    renameTable(table, tableBackup);

    foreach(QString col, columns) {
        sql.remove(QRegularExpression(QString(", \"?%1\"? ?\\w*").arg(col)));
    }

    query.exec(sql);

    QStringList newCols;
    QSqlRecord record = d->database.record(table);
    int count = record.count();
    for(int i = 0; i < count; ++i) {
        newCols << record.fieldName(i);
    }

    query.exec(QString("INSERT INTO %1 SELECT %2 FROM %3")
               .arg(table)
               .arg(newCols.join(','))
               .arg(tableBackup));

    dropTable(tableBackup);

    if(!d->database.commit())
        return false;

    d->database.close();
    d->database.open();
    return true;
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
        createRelationTables(metaObject);
        addMissingColumns(metaObject);
    }
}

QpError QpDatabaseSchema::lastError() const
{
    return d->lastError;
}

bool QpDatabaseSchema::renameColumn(const QString &tableName, const QString &oldColumnName, const QString &newColumnName)
{
    d->database.close();
    d->database.open();

    if(!d->database.transaction())
        return false;

    QpSqlQuery query(d->database);
    query.exec(QString("SELECT sql FROM sqlite_master WHERE name = '%1'")
               .arg(tableName));
    if (!query.first()
         || query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    QString sql = query.value(0).toString();
    sql.replace(QString("\"%1\"").arg(oldColumnName), QString("\"%1\"").arg(newColumnName));
    sql.replace(QString(" %1 ").arg(oldColumnName), QString(" %1 ").arg(newColumnName));
    sql.replace(QString("(%1 ").arg(oldColumnName), QString("(%1 ").arg(newColumnName));

    query.exec("PRAGMA writable_schema = 1;");
    query.exec(QString("UPDATE SQLITE_MASTER SET SQL ="
                       "'%1' WHERE NAME = '%2';")
               .arg(sql)
               .arg(tableName));
    query.exec("PRAGMA writable_schema = 0;");

    if (query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    if(!d->database.commit())
        return false;

    d->database.close();
    d->database.open();
    return true;
}

bool QpDatabaseSchema::renameTable(const QString &oldTableName, const QString &newTableName)
{
    QpSqlQuery query(d->database);
    query.exec(QString("ALTER TABLE %1 RENAME TO %2")
               .arg(oldTableName)
               .arg(newTableName));

    if (query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    return true;
}

bool QpDatabaseSchema::createColumnCopy(const QString &sourceTable,
                                  const QString &sourceColumn,
                                  const QString &destColumn)
{

    if(!d->database.transaction())
        return false;

    QSqlQuery query(d->database);
    QSqlRecord record = d->database.record(sourceTable);
    int i = record.indexOf(sourceColumn);

    QString type = QpDatabaseSchema::variantTypeToSqlType(record.field(i).type());

    query.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3")
               .arg(sourceTable)
               .arg(destColumn)
               .arg(type));
    query.exec(QString("UPDATE %1 SET %2 = %3")
                .arg(sourceTable)
                .arg(destColumn)
                .arg(sourceColumn));

    if (query.lastError().isValid()) {
        setLastError(query);
        return false;
    }

    if(!d->database.commit())
        return false;

    return true;
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
