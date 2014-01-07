#include "sqlcondition.h"

#include <QList>
#include <QSharedData>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

class QpSqlConditionData : public QSharedData {
public:
    QString rawString;
    QString key;
    QVariant value;
    QpSqlCondition::BooleanOperator booleanOperator;
    QpSqlCondition::ComparisonOperator comparisonOperator;
    QList<QpSqlCondition> conditions;
};

QpSqlCondition::QpSqlCondition() :
    data(new QpSqlConditionData)
{
}

QpSqlCondition::QpSqlCondition(const QString &rawString) :
    data(new QpSqlConditionData)
{
    data->rawString = rawString;
}

QpSqlCondition::QpSqlCondition(const QString &key, QpSqlCondition::ComparisonOperator op, const QVariant &value) :
    data(new QpSqlConditionData)
{
    data->key = key;
    data->booleanOperator = And;
    data->comparisonOperator = op;
    data->value = value;
}

QpSqlCondition::QpSqlCondition(QpSqlCondition::BooleanOperator op, const QList<QpSqlCondition> &conditions) :
    data(new QpSqlConditionData)
{
    data->booleanOperator = op;
    data->conditions = conditions;
    data->comparisonOperator = EqualTo;
}

bool QpSqlCondition::isValid() const
{
    return !data->key.isEmpty()
            || (data->booleanOperator == Not
                && data->conditions.size() == 1)
            || !data->conditions.isEmpty();
}

QpSqlCondition::QpSqlCondition(const QpSqlCondition &rhs) :
    data(rhs.data)
{
}

QpSqlCondition &QpSqlCondition::operator=(const QpSqlCondition &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);

    return *this;
}

QpSqlCondition::~QpSqlCondition()
{
}

QpSqlCondition QpSqlCondition::operator !()
{
    QpSqlCondition result(QpSqlCondition::Not, QList<QpSqlCondition>() << *this);
    return result;
}

QpSqlCondition QpSqlCondition::operator ||(const QpSqlCondition &rhs)
{
    return QpSqlCondition(QpSqlCondition::Or, QList<QpSqlCondition>() << *this << rhs);
}

QpSqlCondition QpSqlCondition::operator &&(const QpSqlCondition &rhs)
{
    return QpSqlCondition(QpSqlCondition::And, QList<QpSqlCondition>() << *this << rhs);
}

QString QpSqlCondition::toWhereClause(bool bindValues) const
{
    if (!data->rawString.isEmpty())
        return data->rawString;

    if (data->booleanOperator == Not) {
        Q_ASSERT(data->conditions.size() == 1);

        return booleanOperator().append(data->conditions.first().toWhereClause());
    }

    if (!data->conditions.isEmpty()) {
        QStringList conditions;
        foreach (const QpSqlCondition &condition, data->conditions) {
            conditions.append(condition.toWhereClause(bindValues));
        }

        QString result = conditions.join(booleanOperator());
        if (data->conditions.size() > 1)
            result = result.prepend('(').append(')');

        return result;
    }

    Q_ASSERT(!data->key.isEmpty());

    return comparisonOperator().prepend(QString("\"%1\"").arg(data->key)).append("?");
}

QVariantList QpSqlCondition::bindValues() const
{
    QVariantList result;

    foreach (const QpSqlCondition& condition, data->conditions) {
        result.append(condition.bindValues());
    }

    if (!data->key.isEmpty())
        result.append(data->value);

    return result;
}

QString QpSqlCondition::booleanOperator() const
{
    switch (data->booleanOperator) {
    case And:
        return " AND ";
    case Or:
        return " OR ";
    case Not:
        return "NOT ";
    }
}

QString QpSqlCondition::comparisonOperator() const
{
    switch (data->comparisonOperator) {
    case EqualTo:
        return " = ";
    case GreaterThan:
        return " > ";
    case LessThan:
        return " < ";
    case GreaterThanOrEqualTo:
        return " >= ";
    case LessThanOrEqualTo:
        return " <= ";
    case NotEqualTo:
        return " <> ";
    }
}


