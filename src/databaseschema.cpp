#include "databaseschema.h"

#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlquery.h"

#include <QDebug>
#include <QFile>
#include <QMetaProperty>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>

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
    data(new QpDatabaseSchemaPrivate)
{
    data->database = database;
    data->query = QpSqlQuery(database);
}

QpDatabaseSchema::~QpDatabaseSchema()
{
}

bool QpDatabaseSchema::existsTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());
    return existsTable(meta.tableName());
}

bool QpDatabaseSchema::existsTable(const QString &table)
{
    return data->database.tables().contains(table);
}


void QpDatabaseSchema::createTableIfNotExists(const QMetaObject &metaObject)
{
    if (!existsTable(metaObject))
        createTable(metaObject);
}

void QpDatabaseSchema::dropTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());

    dropTable(meta.tableName());
}

void QpDatabaseSchema::dropTable(const QString &table)
{
    data->query.clear();
    data->query.setTable(table);
    data->query.prepareDropTable();

    if ( !data->query.exec()
         || data->query.lastError().isValid()) {
        setLastError(data->query);
    }
}

void QpDatabaseSchema::createTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());

    data->query.clear();
    data->query.setTable(meta.tableName());

    foreach (QpMetaProperty metaProperty, meta.metaProperties()) {
        Q_ASSERT(metaProperty.isValid());

        if (!metaProperty.isStored())
            continue;

        if (metaProperty.isRelationProperty()) {
            if (metaProperty.tableName() == meta.tableName()) {
                QpMetaProperty reverseRelation = metaProperty.reverseRelation();
                if (reverseRelation.isValid()) {
                    QpMetaObject reverseMetaObject = reverseRelation.metaObject();
                    QString columnName = metaProperty.columnName();
                    QString columnType = QpDatabaseSchema::variantTypeToSqlType(QVariant::Int);

                    data->query.addField(columnName, columnType);
                    data->query.addForeignKey(metaProperty.columnName(),
                                           PRIMARY_KEY_COLUMN_NAME,
                                           reverseMetaObject.tableName());
                }
            }
        }
        else {
            QString columnName = metaProperty.columnName();
            QString columnType = QpDatabaseSchema::variantTypeToSqlType(metaProperty.type());

            data->query.addField(columnName, columnType);
        }
    }

    // Add the primary key
    data->query.addField(PRIMARY_KEY_COLUMN_NAME, "INTEGER PRIMARY KEY AUTOINCREMENT");

    data->query.prepareCreateTable();

    if ( !data->query.exec()
         || data->query.lastError().isValid()) {
        setLastError(data->query);
    }
}

void QpDatabaseSchema::createRelationTables(const QMetaObject &metaObject)
{
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());
    QString primaryTable = meta.tableName();
    QString columnType = QpDatabaseSchema::variantTypeToSqlType(QVariant::Int);

    foreach (QpMetaProperty property, meta.relationProperties()) {
        Q_ASSERT(property.isValid());
        if (property.cardinality() != QpMetaProperty::ManyToManyCardinality)
            continue;

        QString tableName = property.tableName();
        if (data->database.tables().contains(tableName))
            continue;

        QString columnName = property.columnName();
        QString foreignTable = property.reverseMetaObject().tableName();
        QString foreignColumnName = property.reverseRelation().columnName();

        QpSqlQuery createTableQuery(data->database);
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
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());

    foreach (QpMetaProperty metaProperty, meta.metaProperties()) {
        Q_ASSERT(metaProperty.isValid());
        QSqlRecord record = data->database.record(metaProperty.tableName());;

        if (!metaProperty.isStored())
            continue;

        if (metaProperty.isRelationProperty()) {
            QpMetaProperty::Cardinality cardinality = metaProperty.cardinality();
            if (cardinality == QpMetaProperty::ToManyCardinality
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

    if (metaProperty.isRelationProperty()) {
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

    data->query.clear();
    data->query.setTable(table);
    data->query.addField(column, type);
    data->query.prepareAlterTable();

    if ( !data->query.exec()
         || data->query.lastError().isValid()) {
        setLastError(data->query);
    }
}

bool QpDatabaseSchema::dropColumns(const QString &table, const QStringList &columns)
{
    data->database.close();
    data->database.open();

    if (!data->database.transaction())
        return false;

    QpSqlQuery query(data->database);
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

    foreach (QString col, columns) {
        sql.remove(QRegularExpression(QString(", \"?%1\"? ?\\w*").arg(col)));
    }

    query.exec(sql);

    QStringList newCols;
    QSqlRecord record = data->database.record(table);
    int count = record.count();
    for (int i = 0; i < count; ++i) {
        newCols << record.fieldName(i);
    }

    query.exec(QString("INSERT INTO %1 SELECT %2 FROM %3")
               .arg(table)
               .arg(newCols.join(','))
               .arg(tableBackup));

    dropTable(tableBackup);

    if (!data->database.commit())
        return false;

    data->database.close();
    data->database.open();
    return true;
}

void QpDatabaseSchema::createCleanSchema()
{
    QFile file(data->database.databaseName());
    if (file.exists()) {
        if (!file.remove())
            qCritical() << Q_FUNC_INFO << "Could not remove database file"<< file.fileName();
        if (!data->database.open())
            qCritical() << Q_FUNC_INFO << "Could not re-open database file"<< file.fileName();
    }

    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createTable(metaObject.metaObject());
    }
}

void QpDatabaseSchema::adjustSchema()
{
    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createTableIfNotExists(metaObject.metaObject());
        createRelationTables(metaObject.metaObject());
        addMissingColumns(metaObject.metaObject());
    }
}

QpError QpDatabaseSchema::lastError() const
{
    return data->lastError;
}

bool QpDatabaseSchema::renameColumn(const QString &tableName, const QString &oldColumnName, const QString &newColumnName)
{
    data->database.close();
    data->database.open();

    if (!data->database.transaction())
        return false;

    QpSqlQuery query(data->database);
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

    if (!data->database.commit())
        return false;

    data->database.close();
    data->database.open();
    return true;
}

bool QpDatabaseSchema::renameTable(const QString &oldTableName, const QString &newTableName)
{
    QpSqlQuery query(data->database);
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

    if (!data->database.transaction())
        return false;

    QSqlQuery query(data->database);
    QSqlRecord record = data->database.record(sourceTable);
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

    if (!data->database.commit())
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
    data->lastError = error;
}

void QpDatabaseSchema::setLastError(const QSqlQuery &query) const
{
    setLastError(QpError(query.lastError().text().append(": ").append(query.executedQuery()), QpError::SqlError));
}
QString QpDatabaseSchemaPrivate::metaPropertyToColumnDefinition(const QpMetaProperty &metaProperty)
{
    QString name;
    QString type;

    if (metaProperty.isRelationProperty()) {
        name = metaProperty.columnName();
        type = QpDatabaseSchema::variantTypeToSqlType(QVariant::Int);

        switch (metaProperty.cardinality()) {
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

    if (name.isEmpty()
            || type.isEmpty()) {
        return QString();
    }

    return QString("\"%2\" %3")
            .arg(name)
            .arg(type);
}
