#ifndef QPERSISTENCE_SQLCONDITION_H
#define QPERSISTENCE_SQLCONDITION_H

#include <QtCore/QSharedDataPointer>

#include <QtCore/QVariantList>

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
    QpSqlCondition(BooleanOperator op, const QList<QpSqlCondition> &conditions);

    bool isValid() const;

    QpSqlCondition(const QpSqlCondition &);
    QpSqlCondition &operator=(const QpSqlCondition &);
    ~QpSqlCondition();

    QpSqlCondition operator !();
    QpSqlCondition operator ||(const QpSqlCondition &rhs);
    QpSqlCondition operator &&(const QpSqlCondition &rhs);

    QString toWhereClause(bool bindValues = true) const;
    QVariantList bindValues() const;

    QString booleanOperator() const;
    QString comparisonOperator() const;

private:
    QSharedDataPointer<QpSqlConditionData> d;
};

#endif // QPERSISTENCE_SQLCONDITION_H
