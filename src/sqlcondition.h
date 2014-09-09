#ifndef QPERSISTENCE_SQLCONDITION_H
#define QPERSISTENCE_SQLCONDITION_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVariantList>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QVariant;
class QpSqlQuery;

class QpSqlConditionData;
class QpSqlCondition
{
public:
    enum BooleanOperator {
        And,
        Or,
        Not
    };

    enum ComparisonOperator {
        EqualTo,
        GreaterThan,
        LessThan,
        GreaterThanOrEqualTo,
        LessThanOrEqualTo,
        NotEqualTo
    };

    static QpSqlCondition notDeletedAnd(const QpSqlCondition &additionalConditions = QpSqlCondition());
    QpSqlCondition();
    QpSqlCondition(const QString &rawString);
    QpSqlCondition(const QString &field, ComparisonOperator op, const QVariant &value);
    QpSqlCondition(BooleanOperator op, QpSqlCondition &condition);
    QpSqlCondition(BooleanOperator op, const QList<QpSqlCondition> &conditions);

    bool isValid() const;

    void setBindValuesAsString(bool bindValues);
    bool bindValuesAsString() const;

    QpSqlCondition(const QpSqlCondition &);
    QpSqlCondition &operator=(const QpSqlCondition &);
    ~QpSqlCondition();

    QpSqlCondition operator !();
    QpSqlCondition operator ||(const QpSqlCondition &rhs);
    QpSqlCondition operator &&(const QpSqlCondition &rhs);

    QString toWhereClause() const;
    QVariantList bindValues() const;

    QString booleanOperator() const;
    QString comparisonOperator() const;

    void setTable(const QString &table);

private:
    QSharedDataPointer<QpSqlConditionData> data;
};

#endif // QPERSISTENCE_SQLCONDITION_H
