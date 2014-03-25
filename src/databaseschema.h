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
class QpError;
class QpMetaProperty;
class QpSqlQuery;

class QpDatabaseSchemaPrivate;
class QpDatabaseSchema : public QObject
{
    Q_OBJECT
public:
    static const char* COLUMN_NAME_DELETEDFLAG;
    static const char* COLUMN_NAME_PRIMARY_KEY;
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

    explicit QpDatabaseSchema(const QSqlDatabase &database = QSqlDatabase::database(), QObject *parent = 0);
    ~QpDatabaseSchema();

    bool existsTable(const QMetaObject &metaObject);
    bool existsTable(const QString &table);
    void createTable(const QMetaObject &metaObject);
    void createTableIfNotExists(const QMetaObject &metaObject);
    void dropTable(const QMetaObject &metaObject);
    void dropTable(const QString &table);
    bool addMissingColumns(const QMetaObject &metaObject);
    void addColumn(const QpMetaProperty &metaProperty);
    void addColumn(const QString &table, const QString &column, const QString &type);
    bool dropColumns(const QString &table, const QStringList &columns);

    void cleanSchema();
    void createCleanSchema();
    void adjustSchema();

#ifndef QP_NO_LOCKS
    void createLocksTable();
#endif

    QpError lastError() const;

    bool renameColumn(const QString &tableName, const QString &oldColumnName, const QString &newColumnName);
    bool renameTable(const QString &oldTableName, const QString &newTableName);
    bool createColumnCopy(const QString &sourceTable, const QString &sourceColumn, const QString &destColumn);

    QString variantTypeToSqlType(QVariant::Type type);

private:
    QSharedDataPointer<QpDatabaseSchemaPrivate> data;

    void setLastError(const QpError &error) const;
    void setLastError(const QSqlQuery &query) const;

    void createManyToManyRelationTables(const QMetaObject &metaObject);

    QString metaPropertyToColumnDefinition(const QpMetaProperty &metaProperty);
};



#endif // QPERSISTENCE_DATABASESCHEMA_H
