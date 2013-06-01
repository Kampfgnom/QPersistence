#ifndef QPERSISTENCE_DATABASESCHEMAHELPER_H
#define QPERSISTENCE_DATABASESCHEMAHELPER_H

#include <QtCore/QObject>

#include <QtCore/QVariant>
#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>

class QSqlQuery;
class QpError;
class QpMetaProperty;

class QpDatabaseSchemaPrivate;
class QpDatabaseSchema : public QObject
{
    Q_OBJECT
public:
    static const QString PRIMARY_KEY_COLUMN_NAME;

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

    template<class O> bool existsTable() { return existsTable(&O::staticMetaObject); }
    template<class O> void createTable() { createTable(&O::staticMetaObject); }
    template<class O> void createTableIfNotExists() { createTableIfNotExists(&O::staticMetaObject); }
    template<class O> void dropTable() { dropTable(&O::staticMetaObject); }
    template<class O> void addMissingColumns() { addMissingColumns(&O::staticMetaObject); }

    void createCleanSchema();
    void adjustSchema();

    QpError lastError() const;

    bool renameColumn(const QString &tableName, const QString &oldColumnName, const QString &newColumnName);
    bool renameTable(const QString &oldTableName, const QString &newTableName);
    bool createColumnCopy(const QString &sourceTable, const QString &sourceColumn, const QString &destColumn);

    static QString variantTypeToSqlType(QVariant::Type type);

private:
    QSharedDataPointer<QpDatabaseSchemaPrivate> d;

    void setLastError(const QpError &error) const;
    void setLastError(const QSqlQuery &query) const;

    void createRelationTables(const QMetaObject &metaObject);
};



#endif // QPERSISTENCE_DATABASESCHEMAHELPER_H
