#ifndef QPERSISTENCE_CONDITION_H
#define QPERSISTENCE_CONDITION_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVariantList>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QVariant;
class QpSqlQuery;

class QpConditionData;
class QpCondition
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

    static QpCondition notDeletedAnd(const QpCondition &additionalConditions = QpCondition());
    static QpCondition primaryKeys(const QList<int> &primaryKeys);
    QpCondition();
    QpCondition(const QString &rawString);
    QpCondition(const QString &field, ComparisonOperator op, const QVariant &value);
    QpCondition(BooleanOperator op, QpCondition &condition);
    QpCondition(BooleanOperator op, const QList<QpCondition> &conditions);

    bool isValid() const;

    void setBindValuesAsString(bool bindValues);
    bool bindValuesAsString() const;

    QpCondition(const QpCondition &);
    QpCondition &operator=(const QpCondition &);
    ~QpCondition();

    QpCondition operator !();
    QpCondition operator ||(const QpCondition &rhs);
    QpCondition operator &&(const QpCondition &rhs);

    QString toSqlClause() const;
    QVariantList bindValues() const;

    BooleanOperator booleanOperator() const;
    QString booleanOperatorSqlString() const;
    ComparisonOperator comparisonOperator() const;
    QString comparisonOperatorSqlString() const;

    QString table() const;
    void setTable(const QString &table);

private:
    QSharedDataPointer<QpConditionData> data;
};

Q_DECLARE_METATYPE(QpCondition)

#endif // QPERSISTENCE_CONDITION_H
