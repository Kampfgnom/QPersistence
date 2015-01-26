#ifndef QPERSISTENCE_DATABASESCHEMA_H
#define QPERSISTENCE_DATABASESCHEMA_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QSqlQuery;
class QpMetaProperty;
class QpSqlQuery;
class QpStorage;

class QpDatabaseSchemaData;
class QpDatabaseSchema : public QObject
{
    Q_OBJECT
public:
    static const char* COLUMN_NAME_DELETEDFLAG;
    static const char* COLUMN_NAME_PRIMARY_KEY;
    static const char* COLUMN_NAME_REVISION;
    static const char* COLUMN_NAME_ACTION;
    static const char* TABLE_NAME_TEMPLATE_HISTORY;
    static const char* ONDELETE_CASCADE;
#ifndef QP_NO_TIMESTAMPS
    static const char* COLUMN_NAME_CREATION_TIME;
    static const char* COLUMN_NAME_UPDATE_TIME;
#endif
#ifndef QP_NO_LOCKS
    static const char* TABLENAME_LOCKS;
    static const char* COLUMN_LOCK;
    static const char* COLUMN_LOCKTIME;
#endif
#ifndef QP_NO_SCHEMAVERSIONING
    static const char* TABLENAME_SCHEMAVERSIONING;
    static const char* COLUMN_NAME_SCHEMAVERSION_MAJOR;
    static const char* COLUMN_NAME_SCHEMAVERSION_MINOR;
    static const char* COLUMN_NAME_SCHEMAVERSION_DOT;
    static const char* COLUMN_NAME_SCHEMAVERSION_SCRIPT;
    static const char* COLUMN_NAME_SCHEMAVERSION_DATEAPPLIED;
#endif

    explicit QpDatabaseSchema(QpStorage *storage);
    ~QpDatabaseSchema();

    bool existsTable(const QMetaObject &metaObject);
    bool existsTable(const QString &table);
    bool createTable(const QMetaObject &metaObject);
    bool createTableIfNotExists(const QMetaObject &metaObject);
    bool dropTable(const QMetaObject &metaObject);
    bool dropTable(const QString &table);
    bool addMissingColumns(const QMetaObject &metaObject);
    bool addColumn(const QpMetaProperty &metaProperty);
    bool addColumn(const QString &table, const QString &column, const QString &type);
    bool dropColumns(const QString &table, const QStringList &columns);

    bool enableHistoryTracking();
    bool enableHistoryTracking(const QMetaObject &metaObject);
    bool enableHistoryTracking(const QString &table);

    bool cleanSchema();
    bool createCleanSchema();
    bool adjustSchema();

    bool setForeignKeyChecks(bool check);

    bool createManyToManyRelationTables(const QMetaObject &metaObject);

#ifndef QP_NO_LOCKS
    bool createLocksTable();
#endif

#ifndef QP_NO_SCHEMAVERSIONING
    bool createSchemaVersioningTable();
#endif

    bool renameColumn(const QString &tableName, const QString &oldColumnName, const QString &newColumnName);
    bool renameTable(const QString &oldTableName, const QString &newTableName);
    bool createColumnCopy(const QString &sourceTable, const QString &sourceColumn, const QString &destColumn);

    QString variantTypeToSqlType(QVariant::Type type);

private:
    QSharedDataPointer<QpDatabaseSchemaData> data;

    QString metaPropertyToColumnDefinition(const QpMetaProperty &metaProperty);
};



#endif // QPERSISTENCE_DATABASESCHEMA_H
