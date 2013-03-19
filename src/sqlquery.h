#ifndef QPERSISTENCE_SQLQUERY_H
#define QPERSISTENCE_SQLQUERY_H

#include <QtSql/QSqlQuery>

#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QVariant>

class QPersistenceSqlCondition;

class QPersistenceSqlQueryPrivate;
class QPersistenceSqlQuery : public QSqlQuery
{
public:
    enum Order {
        Ascending,
        Descending
    };

    QPersistenceSqlQuery();
    QPersistenceSqlQuery(const QSqlDatabase &database);
    QPersistenceSqlQuery(const QPersistenceSqlQuery &);
    QPersistenceSqlQuery &operator=(const QPersistenceSqlQuery &);
    ~QPersistenceSqlQuery();

    bool exec();

    QString table() const;

    void clear();
    void setTable(const QString &table);
    void addField(const QString &name, const QVariant &value = QVariant());
    void addForeignKey(const QString &columnName, const QString &keyName, const QString &foreignTableName);
    void setLimit(int limit);
    void setWhereCondition(const QPersistenceSqlCondition &condition);
    void addOrder(const QString &field, Order order = Ascending);

    void prepareCreateTable();
    void prepareDropTable();
    void prepareAlterTable();

    void prepareSelect();
    bool prepareUpdate();
    void prepareInsert();
    void prepareDelete();

    void addBindValue(const QVariant &val);

private:
    QExplicitlySharedDataPointer<QPersistenceSqlQueryPrivate> d;
};

#endif // QPERSISTENCE_SQLQUERY_H
