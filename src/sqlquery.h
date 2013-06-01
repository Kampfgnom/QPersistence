#ifndef QPERSISTENCE_SQLQUERY_H
#define QPERSISTENCE_SQLQUERY_H

#include <QtSql/QSqlQuery>

#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QVariant>

class QpSqlCondition;

class QpSqlQueryPrivate;
class QpSqlQuery : public QSqlQuery
{
public:
    enum Order {
        Ascending,
        Descending
    };

    QpSqlQuery();
    QpSqlQuery(const QSqlDatabase &database);
    QpSqlQuery(const QpSqlQuery &);
    QpSqlQuery &operator=(const QpSqlQuery &);
    ~QpSqlQuery();

    bool exec(const QString &queryString);
    bool exec();

    QString table() const;

    void clear();
    void setTable(const QString &table);
    void addField(const QString &name, const QVariant &value = QVariant());
    void addForeignKey(const QString &columnName, const QString &keyName, const QString &foreignTableName);
    void setCount(int limit);
    void setSkip(int skip);
    void setWhereCondition(const QpSqlCondition &condition);
    void addOrder(const QString &field, Order order = Ascending);

    void prepareCreateTable();
    void prepareDropTable();
    void prepareAlterTable();

    void prepareSelect();
    bool prepareUpdate();
    void prepareInsert();
    void prepareDelete();

    void addBindValue(const QVariant &val);

    static QVariant variantToSqlStorableVariant(const QVariant &val);
    static QVariant variantFromSqlStorableVariant(const QVariant &val, QMetaType::Type type);

private:
    QExplicitlySharedDataPointer<QpSqlQueryPrivate> d;

};

#endif // QPERSISTENCE_SQLQUERY_H
