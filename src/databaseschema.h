#ifndef QPERSISTENCE_DATABASESCHEMAHELPER_H
#define QPERSISTENCE_DATABASESCHEMAHELPER_H

#include <QtCore/QObject>

#include <QtCore/QVariant>
#include <QtCore/QSharedDataPointer>
#include <QtSql/QSqlDatabase>

class QSqlQuery;
class QPersistenceError;
class QPersistenceMetaProperty;

class QPersistenceDatabaseSchemaPrivate;
class QPersistenceDatabaseSchema : public QObject
{
    Q_OBJECT
public:
    explicit QPersistenceDatabaseSchema(const QSqlDatabase &database = QSqlDatabase::database(), QObject *parent = 0);
    ~QPersistenceDatabaseSchema();

    bool existsTable(const QMetaObject &metaObject);
    void createTable(const QMetaObject &metaObject);
    void createTableIfNotExists(const QMetaObject &metaObject);
    void dropTable(const QMetaObject &metaObject);
    bool addMissingColumns(const QMetaObject &metaObject);
    void addColumn(const QPersistenceMetaProperty &metaProperty);

    template<class O> bool existsTable() { return existsTable(&O::staticMetaObject); }
    template<class O> void createTable() { createTable(&O::staticMetaObject); }
    template<class O> void createTableIfNotExists() { createTableIfNotExists(&O::staticMetaObject); }
    template<class O> void dropTable() { dropTable(&O::staticMetaObject); }
    template<class O> void addMissingColumns() { addMissingColumns(&O::staticMetaObject); }

    void createCleanSchema();
    void adjustSchema();

    QPersistenceError lastError() const;

    static QString variantTypeToSqlType(QVariant::Type type);

private:
    QSharedDataPointer<QPersistenceDatabaseSchemaPrivate> d;

    void setLastError(const QPersistenceError &error) const;
    void setLastError(const QSqlQuery &query) const;
};

#endif // QPERSISTENCE_DATABASESCHEMAHELPER_H
