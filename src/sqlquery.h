#ifndef QPERSISTENCE_SQLQUERY_H
#define QPERSISTENCE_SQLQUERY_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QVariant>
#include <QtSql/QSqlQuery>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

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
    void addPrimaryKey(const QString &name);
    void addKey(const QString &keyType, const QStringList &fields);
    void setOrIgnore(bool ignore);
    void addRawField(const QString &name, const QString &value = QString());
    void addField(const QString &name, const QVariant &value = QVariant());
    void addForeignKey(const QString &columnName,
                       const QString &keyName,
                       const QString &foreignTableName,
                       const QString &onDelete = QString(),
                       const QString &onUpdate = QString());
    void setCount(int limit);
    void setSkip(int skip);
    void setWhereCondition(const QpSqlCondition &condition);
    void addOrder(const QString &field, Order order = Ascending);
    void setForUpdate(bool forUpdate);
    void addJoin(const QString &direction, const QString &table, const QString &on);
    void addJoin(const QString &direction, const QpSqlQuery &subSelect, const QString &on);
    void addGroupBy(const QString &groupBy);

    void prepareCreateTable();
    void prepareDropTable();
    void prepareAlterTable();

    void prepareSelect();
    bool prepareUpdate();
    void prepareInsert();
    void prepareDelete();
    void prepareincrementNumericColumn();

    QMetaProperty propertyForIndex(const QSqlRecord &record, const QMetaObject *metaObject, int index) const;

    void addBindValue(const QVariant &val);

    static QVariant variantToSqlStorableVariant(const QVariant &val);
    static QVariant variantFromSqlStorableVariant(const QVariant &val, QMetaType::Type type);

    static bool isDebugEnabled();
    static void setDebugEnabled(bool value);
    static void bulkExec();
    static void startBulkExec();

    QString escapedQualifiedField(const QString &field) const;

    static QString escapeField(const QString &field);

private:
    QExplicitlySharedDataPointer<QpSqlQueryPrivate> data;

};

#endif // QPERSISTENCE_SQLQUERY_H
