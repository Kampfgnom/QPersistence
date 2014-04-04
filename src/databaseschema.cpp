#include "databaseschema.h"

#include "error.h"
#include "lock.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDebug>
#include <QFile>
#include <QMetaProperty>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>
#include <QSqlDriver>
#include <QtCore/qmath.h>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

const char* QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG("_Qp_deleted");
const char* QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY("_Qp_ID");
const char* QpDatabaseSchema::ONDELETE_CASCADE("CASCADE");
#ifndef QP_NO_TIMESTAMPS
const char* QpDatabaseSchema::COLUMN_NAME_CREATION_TIME("_Qp_creationTime");
const char* QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME("_Qp_updateTime");
#endif
#ifndef QP_NO_LOCKS
const char* QpDatabaseSchema::TABLENAME_LOCKS("_Qp_locks");
const char* QpDatabaseSchema::COLUMN_LOCK("_Qp_lock");
#endif

class QpDatabaseSchemaPrivate : public QSharedData
{
public:
    QpDatabaseSchemaPrivate() :
        QSharedData()
    {}

    QSqlDatabase database;
    mutable QpError lastError;
    QpSqlQuery query;
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
                    QString columnType = variantTypeToSqlType(QVariant::Int);

                    data->query.addField(columnName, columnType);
                    data->query.addForeignKey(metaProperty.columnName(),
                                              COLUMN_NAME_PRIMARY_KEY,
                                              reverseMetaObject.tableName(),
                                              "SET NULL");
                }
            }
        }
        else if(metaProperty.metaProperty().isEnumType()) {
            QString columnName = metaProperty.columnName();
            QString columnType = variantTypeToSqlType(QVariant::Int);

            if(data->database.driverName() == QLatin1String("QMYSQL")) {
                QMetaEnum metaEnum = metaProperty.metaProperty().enumerator();
                int count = metaEnum.keyCount();
                int i = 0;
                int offset = 0;

                bool isFlag = metaProperty.metaProperty().isFlagType();

                 // Start at 1 because first value corresponds to MySQL empty string
                if(metaEnum.value(0) == 0) {
                    i = 1;
                    offset = 1;
                }

                QStringList enumValues;
                for(; i < count; ++i) {

                    if(isFlag && metaEnum.value(i) != qPow(2, i - offset))
                        break;

                    enumValues << QString("'%1'").arg(metaEnum.key(i));
                }

                columnType = "ENUM(%1)";
                if(isFlag)
                    columnType = "SET(%1)";

                columnType = columnType.arg(enumValues.join(", "));
            }

            data->query.addField(columnName, columnType);
        }
        else {
            QString columnName = metaProperty.columnName();
            QString columnType = metaProperty.attributes().value("columnDefinition");

            if(columnType.isEmpty())
                columnType = variantTypeToSqlType(metaProperty.type());

            data->query.addField(columnName, columnType);
        }
    }

    foreach (QpMetaProperty metaProperty, meta.metaProperties()) {
        QString key = metaProperty.attributes().value("key");
        if(key.isEmpty())
            continue;

        data->query.addKey(key, QStringList() << metaProperty.columnName());
    }

    // Add the primary key
    data->query.addPrimaryKey(COLUMN_NAME_PRIMARY_KEY);
    data->query.addField(COLUMN_NAME_DELETEDFLAG, "BOOLEAN DEFAULT false");

#ifndef QP_NO_TIMESTAMPS
    // Add timestamp columns
    data->query.addField(COLUMN_NAME_CREATION_TIME, variantTypeToSqlType(QVariant::Double));
    data->query.addField(COLUMN_NAME_UPDATE_TIME, variantTypeToSqlType(QVariant::Double));
#endif


#ifndef QP_NO_LOCKS
    data->query.addField(QpDatabaseSchema::COLUMN_LOCK, variantTypeToSqlType(QVariant::Int));
#endif

    data->query.prepareCreateTable();

    if ( !data->query.exec()
         || data->query.lastError().isValid()) {
        setLastError(data->query);
    }
}

void QpDatabaseSchema::createManyToManyRelationTables(const QMetaObject &metaObject)
{
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());
    QString primaryTable = meta.tableName();
    QString columnType = variantTypeToSqlType(QVariant::Int);

    foreach (QpMetaProperty property, meta.relationProperties()) {
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
                                       COLUMN_NAME_PRIMARY_KEY,
                                       primaryTable,
                                       "SET NULL");
        createTableQuery.addForeignKey(foreignColumnName,
                                       COLUMN_NAME_PRIMARY_KEY,
                                       foreignTable,
                                       "SET NULL");
        createTableQuery.addPrimaryKey(COLUMN_NAME_PRIMARY_KEY);
        createTableQuery.addKey(QpSqlBackend::forDatabase(data->database)->uniqueKeyType(),
                                QStringList() << columnName << foreignColumnName);
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
        QSqlRecord record = data->database.record(metaProperty.tableName());

        if (!metaProperty.isStored())
            continue;

        if (metaProperty.isRelationProperty()) {
            QpMetaProperty::Cardinality cardinality = metaProperty.cardinality();
            if (cardinality == QpMetaProperty::OneToManyCardinality) {
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

#if !defined QP_NO_LOCKS || !defined QP_NO_TIMESTAMPS
    // Check for special columns
    QSqlRecord record = data->database.record(meta.tableName());

#ifndef QP_NO_TIMESTAMPS
    bool hasCreationTimeColumn = record.contains(COLUMN_NAME_CREATION_TIME);
    bool hasUpdateTimeColumn = record.contains(COLUMN_NAME_UPDATE_TIME);
#endif

#ifndef QP_NO_LOCKS
    bool hasLockColumn = record.contains(COLUMN_LOCK);
#endif

    if(!hasCreationTimeColumn
            || !hasUpdateTimeColumn) {
        data->query.clear();
        data->query.setTable(meta.tableName());

#ifndef QP_NO_TIMESTAMPS
        // Add timestamp columns
        if(!hasCreationTimeColumn)
            data->query.addField(COLUMN_NAME_CREATION_TIME, variantTypeToSqlType(QVariant::Double));

        if(!hasUpdateTimeColumn)
            data->query.addField(COLUMN_NAME_UPDATE_TIME, variantTypeToSqlType(QVariant::Double));
#endif

#ifndef QP_NO_LOCKS
        if(!hasLockColumn && QpLock::isLocksEnabled())
            data->query.addField(COLUMN_LOCK, variantTypeToSqlType(QVariant::Int));
#endif

        data->query.prepareAlterTable();

        if ( !data->query.exec()
             || data->query.lastError().isValid()) {
            setLastError(data->query);
            return false;
        }
    }
#endif

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
        type = variantTypeToSqlType(QVariant::Int);

        qWarning("The relation %s will be added to the %s table. "
                 "But SQLite does not support adding foreign key contraints after a table has been created. "
                 "You might want to alter the table manually (by creating a new table and copying the original data).",
                 qPrintable(name),
                 qPrintable(tableName));
    }
    else {
        tableName = metaProperty.metaObject().tableName();
        name = metaProperty.columnName();
        type = variantTypeToSqlType(metaProperty.type());
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

void QpDatabaseSchema::cleanSchema()
{
#ifdef QP_FOR_SQLITE
    QFile file(data->database.databaseName());
    if (file.exists()) {
        if (!file.remove())
            qCritical() << Q_FUNC_INFO << "Could not remove database file"<< file.fileName();
        if (!data->database.open())
            qCritical() << Q_FUNC_INFO << "Could not re-open database file"<< file.fileName();
    }
#elif defined QP_FOR_MYSQL
    QpSqlQuery query(data->database);
    query.prepare(QString("DROP SCHEMA IF EXISTS %1").arg(data->database.databaseName()));

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
    }
    query.clear();
    query.prepare(QString("CREATE SCHEMA %1 CHARACTER SET %2")
                  .arg(data->database.databaseName())
                  .arg("utf8"));

    if (!query.exec()
            || query.lastError().isValid()) {
        setLastError(query);
    }

    data->database.close();
    data->database.open();
#else
    foreach(QString table, data->database.tables(QSql::Tables)) {
        dropTable(table);
    }
#endif
}

void QpDatabaseSchema::createCleanSchema()
{
    cleanSchema();

#ifndef QP_NO_LOCKS
    createLocksTable();
#endif

    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createTable(metaObject.metaObject());
    }

    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createManyToManyRelationTables(metaObject.metaObject());
    }
}

void QpDatabaseSchema::adjustSchema()
{
#ifndef QP_NO_LOCKS
    createLocksTable();
#endif

    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createTableIfNotExists(metaObject.metaObject());
        createManyToManyRelationTables(metaObject.metaObject());
        addMissingColumns(metaObject.metaObject());
    }
}

#ifndef QP_NO_LOCKS
void QpDatabaseSchema::createLocksTable()
{
    if (!QpLock::isLocksEnabled())
        return;

    if (!existsTable(QpDatabaseSchema::TABLENAME_LOCKS)) {

        data->query.clear();
        data->query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
        data->query.addPrimaryKey(COLUMN_NAME_PRIMARY_KEY);

        foreach(QString field, QpLock::additionalInformationFields().keys()) {
            QString type = variantTypeToSqlType(QpLock::additionalInformationFields().value(field));
            data->query.addField(field, type);
        }

        data->query.prepareCreateTable();

        if ( !data->query.exec()
             || data->query.lastError().isValid()) {
            setLastError(data->query);
        }
    }
    else {
        QSqlRecord record = data->database.record(QpDatabaseSchema::TABLENAME_LOCKS);

        foreach(QString field, QpLock::additionalInformationFields().keys()) {
            if (record.indexOf(field) != -1)
                continue;

            QString type = variantTypeToSqlType(QpLock::additionalInformationFields().value(field));
            addColumn(QpDatabaseSchema::TABLENAME_LOCKS, field, type);
        }
    }
}
#endif

QpError QpDatabaseSchema::lastError() const
{
    return data->lastError;
}

bool QpDatabaseSchema::renameColumn(const QString &tableName, const QString &oldColumnName, const QString &newColumnName)
{
#ifndef QP_FOR_SQLITE
    Q_ASSERT_X(false, Q_FUNC_INFO, "Renaming columns is currently only supported on QP_FOR_SQLITE databases.");
#endif

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

    QString type = variantTypeToSqlType(record.field(i).type());

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
    return QpSqlBackend::forDatabase(data->database)->variantTypeToSqlType(type);
}

void QpDatabaseSchema::setLastError(const QpError &error) const
{
    qDebug() << error;
    data->lastError = error;
    Qp::Private::setLastError(error);
}

void QpDatabaseSchema::setLastError(const QSqlQuery &query) const
{
    setLastError(QpError(query.lastError().text().append(": ").append(query.executedQuery()), QpError::SqlError));
}

QString QpDatabaseSchema::metaPropertyToColumnDefinition(const QpMetaProperty &metaProperty)
{
    QString name;
    QString type;

    if (metaProperty.isRelationProperty()) {
        name = metaProperty.columnName();
        type = variantTypeToSqlType(QVariant::Int);

        switch (metaProperty.cardinality()) {
        case QpMetaProperty::OneToOneCardinality:
        case QpMetaProperty::ManyToOneCardinality:
            // My table gets a foreign key column
            break;
        case QpMetaProperty::OneToManyCardinality:
            // The related table gets a foreign key column
            return QString();
        case QpMetaProperty::ManyToManyCardinality:
            // The relation need a whole table
            break;

        case QpMetaProperty::UnknownCardinality:
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
        type = variantTypeToSqlType(metaProperty.type());
    }

    if (name.isEmpty()
            || type.isEmpty()) {
        return QString();
    }

    return QString("\"%2\" %3")
            .arg(name)
            .arg(type);
}
