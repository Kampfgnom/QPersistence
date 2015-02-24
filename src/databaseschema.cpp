#include "databaseschema.h"

#include "error.h"
#include "lock.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqlquery.h"
#include "storage.h"

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
const char* QpDatabaseSchema::COLUMN_NAME_REVISION("_Qp_revision");
const char* QpDatabaseSchema::COLUMN_NAME_ACTION("_Qp_action");
const char* QpDatabaseSchema::TABLE_NAME_TEMPLATE_HISTORY("%1_Qp_history");
#ifndef QP_NO_TIMESTAMPS
const char* QpDatabaseSchema::COLUMN_NAME_CREATION_TIME("_Qp_creationTime");
const char* QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME("_Qp_updateTime");
#endif
#ifndef QP_NO_LOCKS
const char* QpDatabaseSchema::TABLENAME_LOCKS("_Qp_locks");
const char* QpDatabaseSchema::COLUMN_LOCK("_Qp_lock");
#endif

#ifndef QP_NO_SCHEMAVERSIONING
const char* QpDatabaseSchema::TABLENAME_SCHEMAVERSIONING("_Qp_SchemaVersion");
const char* QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MAJOR("major");
const char* QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MINOR("minor");
const char* QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DOT("dot");
const char* QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_SCRIPT("script");
const char* QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DATEAPPLIED("date_applied");
#endif


/******************************************************************************
 * QpDatabaseSchemaData
 */
class QpDatabaseSchemaData : public QSharedData
{
public:
    QpDatabaseSchemaData() :
        QSharedData()
    {
    }

    QpStorage *storage;
    QSqlDatabase database;
    QpSqlQuery query;
};


/******************************************************************************
 * QpDatabaseSchema
 */
QpDatabaseSchema::QpDatabaseSchema(QpStorage *storage) :
    data(new QpDatabaseSchemaData)
{
    data->storage = storage;
    data->database = storage->database();
    data->query = QpSqlQuery(data->database);
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

bool QpDatabaseSchema::createTableIfNotExists(const QMetaObject &metaObject)
{
    if (existsTable(metaObject))
        return true;

    return createTable(metaObject);
}

bool QpDatabaseSchema::dropTable(const QMetaObject &metaObject)
{
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());

    return dropTable(meta.tableName());
}

bool QpDatabaseSchema::dropTable(const QString &table)
{
    data->query.clear();
    data->query.setTable(table);
    data->query.prepareDropTable();

    if (!data->query.exec()) {
        data->storage->setLastError(data->query);
        return false;
    }
    return true;
}

bool QpDatabaseSchema::createTable(const QMetaObject &metaObject)
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
        else if (metaProperty.metaProperty().isEnumType()) {
            QString columnName = metaProperty.columnName();
            QString columnType = variantTypeToSqlType(QVariant::Int);

            if (data->database.driverName() == QLatin1String("QMYSQL")) {
                QMetaEnum metaEnum = metaProperty.metaProperty().enumerator();
                int count = metaEnum.keyCount();
                int i = 0;
                int offset = 0;

                bool isFlag = metaProperty.metaProperty().isFlagType();

                // Start at 1 because first value corresponds to MySQL empty string
                if (metaEnum.value(0) == 0) {
                    i = 1;
                    offset = 1;
                }

                QStringList enumValues;
                for (; i < count; ++i) {

                    if (isFlag && metaEnum.value(i) != static_cast<int>(qPow(2, i - offset)))
                        break;

                    enumValues << QString("'%1'").arg(metaEnum.key(i));
                }

                columnType = "ENUM(%1)";
                if (isFlag)
                    columnType = "SET(%1)";

                columnType = columnType.arg(enumValues.join(", "));
            }

            data->query.addField(columnName, columnType);
        }
        else {
            QString columnName = metaProperty.columnName();
            QString columnType = metaProperty.attributes().value("columnDefinition");

            if (columnType.isEmpty())
                columnType = variantTypeToSqlType(metaProperty.type());

            data->query.addField(columnName, columnType);
        }
    }

    foreach (QpMetaProperty metaProperty, meta.metaProperties()) {
        QString key = metaProperty.attributes().value("key");
        if (key.isEmpty())
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
    if (data->storage->isLocksEnabled()) {
        data->query.addField(QpDatabaseSchema::COLUMN_LOCK, variantTypeToSqlType(QVariant::Int));
        data->query.addForeignKey(QpDatabaseSchema::COLUMN_LOCK,
                                  COLUMN_NAME_PRIMARY_KEY,
                                  TABLENAME_LOCKS,
                                  "SET NULL");
    }
#endif

    data->query.prepareCreateTable();

    if (!data->query.exec()) {
        data->storage->setLastError(data->query);
        return false;
    }

    return true;
}

bool QpDatabaseSchema::createManyToManyRelationTables(const QMetaObject &metaObject)
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
                                       ONDELETE_CASCADE);
        createTableQuery.addForeignKey(foreignColumnName,
                                       COLUMN_NAME_PRIMARY_KEY,
                                       foreignTable,
                                       ONDELETE_CASCADE);
        createTableQuery.addPrimaryKey(COLUMN_NAME_PRIMARY_KEY);
        createTableQuery.addKey(QpSqlBackend::forDatabase(data->database)->uniqueKeyType(),
                                QStringList() << columnName << foreignColumnName);
        createTableQuery.prepareCreateTable();
        if (!createTableQuery.exec()) {
            data->storage->setLastError(createTableQuery);
            return false;
        }
    }

    return true;
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

        if (!addColumn(metaProperty))
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

    if (!hasCreationTimeColumn
        || !hasUpdateTimeColumn) {
        data->query.clear();
        data->query.setTable(meta.tableName());

#ifndef QP_NO_TIMESTAMPS
        // Add timestamp columns
        if (!hasCreationTimeColumn)
            data->query.addField(COLUMN_NAME_CREATION_TIME, variantTypeToSqlType(QVariant::Double));

        if (!hasUpdateTimeColumn)
            data->query.addField(COLUMN_NAME_UPDATE_TIME, variantTypeToSqlType(QVariant::Double));
#endif

#ifndef QP_NO_LOCKS
        if (!hasLockColumn && data->storage->isLocksEnabled())
            data->query.addField(COLUMN_LOCK, variantTypeToSqlType(QVariant::Int));
#endif

        data->query.prepareAlterTable();

        if (!data->query.exec()) {
            data->storage->setLastError(data->query);
            return false;
        }
    }
#endif

    return true;
}

bool QpDatabaseSchema::addColumn(const QpMetaProperty &metaProperty)
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

    return addColumn(tableName, name, type);
}

bool QpDatabaseSchema::addColumn(const QString &table, const QString &column, const QString &type)
{
    data->query.clear();
    data->query.setTable(table);
    data->query.addField(column, type);
    data->query.prepareAlterTable();

    if (!data->query.exec()) {
        data->storage->setLastError(data->query);
        return false;
    }

    return true;
}

bool QpDatabaseSchema::dropColumns(const QString &table, const QStringList &columns)
{
    data->database.close();

    if (!data->database.open()
        || !data->storage->beginTransaction())
        return false;

    QpSqlQuery query(data->database);
    if (!query.exec(QString("SELECT sql FROM sqlite_master WHERE name = '%1'")
                    .arg(table))
        || !query.first()) {
        data->storage->setLastError(query);
        return false;
    }

    QString sql = query.value(0).toString();

    QString tableBackup = QString("_Qp_BACKUP_").append(table);
    renameTable(table, tableBackup);

    foreach (QString col, columns) {
        sql.remove(QRegularExpression(QString(", \"?%1\"? ?\\w*").arg(col)));
    }

    if (!query.exec(sql)) {
        data->storage->setLastError(query);
        return false;
    }

    QStringList newCols;
    QSqlRecord record = data->database.record(table);
    int count = record.count();
    for (int i = 0; i < count; ++i) {
        newCols << record.fieldName(i);
    }

    if (!query.exec(QString("INSERT INTO %1 SELECT %2 FROM %3")
                    .arg(table)
                    .arg(newCols.join(','))
                    .arg(tableBackup))) {
        data->storage->setLastError(query);
        return false;
    }

    bool result = dropTable(tableBackup);

    if (!data->storage->commitOrRollbackTransaction())
        return false;

    data->database.close();
    data->database.open();
    return result;
}

bool QpDatabaseSchema::enableHistoryTracking()
{
    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        if (!enableHistoryTracking(metaObject.metaObject()))
            return false;
    }
    return true;
}

bool QpDatabaseSchema::enableHistoryTracking(const QMetaObject &metaObject)
{
    QpMetaObject meta = QpMetaObject::forClassName(metaObject.className());
    QString table = meta.tableName();
    return enableHistoryTracking(table);
}

bool QpDatabaseSchema::enableHistoryTracking(const QString &table)
{
    if (!data->storage->beginTransaction()) {
        data->storage->setLastError(QpError(data->database.lastError()));
        return false;
    }

    QpSqlQuery query(data->database);

    if (!query.exec(QString::fromLatin1(
                            "CREATE TABLE IF NOT EXISTS `%1_Qp_history` ("
                            "`_Qp_revision` INTEGER PRIMARY KEY AUTO_INCREMENT,"
                            "`_Qp_ID` INTEGER NOT NULL, "
                            "`_Qp_action` ENUM('INSERT', 'UPDATE', 'MARK_AS_DELETE', 'DELETE') DEFAULT 'INSERT',"
                            "`datetime` DOUBLE NOT NULL,"
                            "`user` VARCHAR(100) NOT NULL,"
                            "UNIQUE KEY (`_Qp_ID`, `_Qp_revision`));")
                    .arg(table))
        || !query.exec(QString::fromLatin1(
                               "CREATE TRIGGER `%1_Qp_history_INSERT` AFTER INSERT ON `%1` FOR EACH ROW "
                               "INSERT INTO `%1_Qp_history` VALUES ("
                               "NULL, "
                               "NEW.`_Qp_ID`,"
                               "'INSERT', "
                               "NOW(6) + 0,"
                               "CURRENT_USER()"
                               ");")
                       .arg(table))
        || !query.exec(QString::fromLatin1(
                               "CREATE TRIGGER `%1_Qp_history_UPDATE` AFTER UPDATE ON `%1` FOR EACH ROW "
                               "INSERT INTO `%1_Qp_history` VALUES ("
                               "NULL, "
                               "NEW.`_Qp_ID`,"
                               "CASE WHEN NEW.`_Qp_deleted` = 0 THEN 'UPDATE' ELSE 'MARK_AS_DELETE' END, "
                               "NOW(6) + 0,"
                               "CURRENT_USER()"
                               ");")
                       .arg(table))
        || !query.exec(QString::fromLatin1(
                               "CREATE TRIGGER `%1_Qp_history_DELETE` BEFORE DELETE ON `%1` FOR EACH ROW "
                               "INSERT INTO `%1_Qp_history` VALUES ("
                               "NULL, "
                               "OLD.`_Qp_ID`,"
                               "'DELETE', "
                               "NOW(6) + 0,"
                               "CURRENT_USER()"
                               ");")
                       .arg(table))) {
        data->storage->setLastError(query);
        data->database.rollback();
        return false;
    }

    if (!data->storage->commitOrRollbackTransaction()) {
        data->storage->setLastError(QpError(data->database.lastError()));
        return false;
    }

    return true;
}

bool QpDatabaseSchema::cleanSchema()
{
#ifdef QP_FOR_SQLITE
    QFile file(data->database.databaseName());
    if (file.exists()) {
        if (!file.remove()) {
            qCritical() << Q_FUNC_INFO << "Could not remove database file"<< file.fileName();
            return false;
        }
        if (!data->database.open()) {
            qCritical() << Q_FUNC_INFO << "Could not re-open database file"<< file.fileName();
            return false;
        }
    }
#elif defined QP_FOR_MYSQL

    if (!data->query.exec(QString("DROP SCHEMA IF EXISTS %1")
                          .arg(data->database.databaseName()))) {
        data->storage->setLastError(data->query);
        return false;
    }

    if (!data->query.exec(QString("CREATE SCHEMA %1 CHARACTER SET %2")
                          .arg(data->database.databaseName())
                          .arg("utf8"))) {
        data->storage->setLastError(data->query);
        return false;
    }

    data->database.close();
    data->database.open();
#else
    foreach (QString table, data->database.tables(QSql::Tables)) {
        if (!dropTable(table))
            return false;
    }
#endif

    return true;
}

bool QpDatabaseSchema::createCleanSchema()
{
    if (!data->storage->beginTransaction())
        return false;

    setForeignKeyChecks(false);
    cleanSchema();

#ifndef QP_NO_LOCKS
    createLocksTable();
#endif

    createSchemaVersioningTable();

    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createTable(metaObject.metaObject());
        enableHistoryTracking(metaObject.metaObject());
    }

    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createManyToManyRelationTables(metaObject.metaObject());
    }
    setForeignKeyChecks(true);

    return data->storage->commitOrRollbackTransaction();
}

bool QpDatabaseSchema::adjustSchema()
{
    if (!data->storage->beginTransaction())
        return false;

    setForeignKeyChecks(false);
#ifndef QP_NO_LOCKS
    if (data->storage->isLocksEnabled())
        createLocksTable();
#endif
#ifndef QP_NO_SCHEMAVERSIONING
    createSchemaVersioningTable();
#endif

    foreach (const QpMetaObject &metaObject, QpMetaObject::registeredMetaObjects()) {
        createTableIfNotExists(metaObject.metaObject());
        createManyToManyRelationTables(metaObject.metaObject());
        addMissingColumns(metaObject.metaObject());
    }
    setForeignKeyChecks(true);

    return data->storage->commitOrRollbackTransaction();
}

bool QpDatabaseSchema::setForeignKeyChecks(bool check)
{
    QString q;

#ifdef QP_FOR_MYSQL
    if (check)
        q = QString::fromLatin1("SET foreign_key_checks = 1");
    else
        q = QString::fromLatin1("SET foreign_key_checks = 0");
#elif defined QP_FOR_SQLITE
    if (check)
        q = QString::fromLatin1("PRAGMA foreign_keys = ON");
    else
        q = QString::fromLatin1("PRAGMA foreign_keys = OFF");
#endif

    if (!data->query.exec(q)) {
        data->storage->setLastError(data->query);
        return false;
    }
    return true;
}

#ifndef QP_NO_LOCKS
bool QpDatabaseSchema::createLocksTable()
{
    if (!existsTable(QpDatabaseSchema::TABLENAME_LOCKS)) {
        data->query.clear();
        data->query.setTable(QpDatabaseSchema::TABLENAME_LOCKS);
        data->query.addPrimaryKey(COLUMN_NAME_PRIMARY_KEY);

        foreach (QString field, data->storage->additionalLockInformationFields().keys()) {
            QString type = variantTypeToSqlType(data->storage->additionalLockInformationFields().value(field));
            data->query.addField(field, type);
        }

        data->query.prepareCreateTable();

        if (!data->query.exec()) {
            data->storage->setLastError(data->query);
            return false;
        }
    }
    else {
        QSqlRecord record = data->database.record(QpDatabaseSchema::TABLENAME_LOCKS);

        foreach (QString field, data->storage->additionalLockInformationFields().keys()) {
            if (record.indexOf(field) != -1)
                continue;

            QString type = variantTypeToSqlType(data->storage->additionalLockInformationFields().value(field));
            if (!addColumn(QpDatabaseSchema::TABLENAME_LOCKS, field, type))
                return false;
        }
    }
    return true;
}
#endif

#ifndef QP_NO_SCHEMAVERSIONING
bool QpDatabaseSchema::createSchemaVersioningTable()
{
    if (!existsTable(QpDatabaseSchema::TABLENAME_SCHEMAVERSIONING)) {
        data->query.clear();
        data->query.setTable(QpDatabaseSchema::TABLENAME_SCHEMAVERSIONING);
        data->query.addPrimaryKey(COLUMN_NAME_PRIMARY_KEY);

        QString intType = variantTypeToSqlType(QVariant::Int);
        data->query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MAJOR, intType);
        data->query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MINOR, intType);
        data->query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DOT, intType);
        data->query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_SCRIPT, variantTypeToSqlType(QVariant::String));
        data->query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DATEAPPLIED, variantTypeToSqlType(QVariant::DateTime));

        data->query.prepareCreateTable();

        if (!data->query.exec()) {
            data->storage->setLastError(data->query);
            return false;
        }
    }
    return true;
}
#endif

bool QpDatabaseSchema::renameColumn(const QString &tableName, const QString &oldColumnName, const QString &newColumnName)
{
#ifndef QP_FOR_SQLITE
    Q_ASSERT_X(false, Q_FUNC_INFO, "Renaming columns is currently only supported on QP_FOR_SQLITE databases.");
#endif

    data->database.close();
    data->database.open();

    if (!data->storage->beginTransaction())
        return false;

    if (!data->query.exec(QString("SELECT sql FROM sqlite_master WHERE name = '%1'")
                          .arg(tableName))
        || !data->query.first()) {
        data->storage->setLastError(data->query);
        return false;
    }

    QString sql = data->query.value(0).toString();
    sql.replace(QString("\"%1\"").arg(oldColumnName), QString("\"%1\"").arg(newColumnName));
    sql.replace(QString(" %1 ").arg(oldColumnName), QString(" %1 ").arg(newColumnName));
    sql.replace(QString("(%1 ").arg(oldColumnName), QString("(%1 ").arg(newColumnName));

    if (!data->query.exec("PRAGMA writable_schema = 0")) {
        data->storage->setLastError(data->query);
        return false;
    }

    if (!data->query.exec(QString("UPDATE SQLITE_MASTER SET SQL ="
                                  "'%1' WHERE NAME = '%2'")
                          .arg(sql)
                          .arg(tableName))) {
        data->storage->setLastError(data->query);
        return false;
    }

    if (!data->query.exec("PRAGMA writable_schema = 0")) {
        data->storage->setLastError(data->query);
        return false;
    }

    if (!data->storage->commitOrRollbackTransaction())
        return false;

    data->database.close();
    data->database.open();
    return true;
}

bool QpDatabaseSchema::renameTable(const QString &oldTableName, const QString &newTableName)
{
    if (data->query.exec(QString("ALTER TABLE %1 RENAME TO %2")
                         .arg(oldTableName)
                         .arg(newTableName))) {
        data->storage->setLastError(data->query);
        return false;
    }

    return true;
}

bool QpDatabaseSchema::createColumnCopy(const QString &sourceTable,
                                        const QString &sourceColumn,
                                        const QString &destColumn)
{
    if (!data->storage->beginTransaction())
        return false;

    QSqlRecord record = data->database.record(sourceTable);
    int i = record.indexOf(sourceColumn);

    QString type = variantTypeToSqlType(record.field(i).type());
    if (data->query.exec(QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                         .arg(sourceTable)
                         .arg(destColumn)
                         .arg(type))) {
        data->storage->setLastError(data->query);
        return false;
    }
    if (data->query.exec(QString("UPDATE %1 SET %2 = %3")
                         .arg(sourceTable)
                         .arg(destColumn)
                         .arg(sourceColumn))) {
        data->storage->setLastError(data->query);
        return false;
    }

    if (!data->storage->commitOrRollbackTransaction())
        return false;

    return true;
}

QString QpDatabaseSchema::variantTypeToSqlType(QVariant::Type type)
{
    return QpSqlBackend::forDatabase(data->database)->variantTypeToSqlType(type);
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
