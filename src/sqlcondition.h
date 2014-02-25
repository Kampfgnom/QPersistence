#ifndef QPERSISTENCE_SQLCONDITION_H
#define QPERSISTENCE_SQLCONDITION_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVariantList>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QVariant;

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

    QpSqlCondition();
    QpSqlCondition(const QString &rawString);
    QpSqlCondition(const QString &key, ComparisonOperator op, const QVariant &value);
    QpSqlCondition(BooleanOperator op, QpSqlCondition &condition);
    QpSqlCondition(BooleanOperator op, const QList<QpSqlCondition> &conditions);

    bool isValid() const;

    void setBindValuesAsString(bool bindValues);

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

private:
    QSharedDataPointer<QpSqlConditionData> data;
};

#endif // QPERSISTENCE_SQLCONDITION_H
