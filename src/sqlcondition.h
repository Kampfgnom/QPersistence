#ifndef QPERSISTENCE_SQLCONDITION_H
#define QPERSISTENCE_SQLCONDITION_H

#include <QtCore/QSharedDataPointer>

#include <QtCore/QVariantList>

class QVariant;

class QPersistenceSqlConditionData;
class QPersistenceSqlCondition
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

    QPersistenceSqlCondition();
    QPersistenceSqlCondition(const QString &key, ComparisonOperator op, const QVariant &value);
    QPersistenceSqlCondition(BooleanOperator op, const QList<QPersistenceSqlCondition> &conditions);

    bool isValid() const;

    QPersistenceSqlCondition(const QPersistenceSqlCondition &);
    QPersistenceSqlCondition &operator=(const QPersistenceSqlCondition &);
    ~QPersistenceSqlCondition();

    QPersistenceSqlCondition operator !();
    QPersistenceSqlCondition operator ||(const QPersistenceSqlCondition &rhs);
    QPersistenceSqlCondition operator &&(const QPersistenceSqlCondition &rhs);

    QString toWhereClause(bool bindValues = true) const;
    QVariantList bindValues() const;

    QString booleanOperator() const;
    QString comparisonOperator() const;

private:
    QSharedDataPointer<QPersistenceSqlConditionData> d;
};

#endif // QPERSISTENCE_SQLCONDITION_H
