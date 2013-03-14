#include "sqlcondition.h"

#include <QSharedData>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <QVariantList>



class QPersistenceSqlConditionData : public QSharedData {
public:
    QString key;
    QVariant value;
    QPersistenceSqlCondition::BooleanOperator booleanOperator;
    QPersistenceSqlCondition::ComparisonOperator comparisonOperator;
    QList<QPersistenceSqlCondition> conditions;
};

QPersistenceSqlCondition::QPersistenceSqlCondition() :
    d(new QPersistenceSqlConditionData)
{
}

QPersistenceSqlCondition::QPersistenceSqlCondition(const QString &key, QPersistenceSqlCondition::ComparisonOperator op, const QVariant &value) :
    d(new QPersistenceSqlConditionData)
{
    d->key = key;
    d->booleanOperator = And;
    d->comparisonOperator = op;
    d->value = value;
}

QPersistenceSqlCondition::QPersistenceSqlCondition(QPersistenceSqlCondition::BooleanOperator op, const QList<QPersistenceSqlCondition> &conditions) :
    d(new QPersistenceSqlConditionData)
{
    d->booleanOperator = op;
    d->conditions = conditions;
    d->comparisonOperator = EqualTo;
}

bool QPersistenceSqlCondition::isValid() const
{
    return !d->key.isEmpty()
            || (d->booleanOperator == Not
                && d->conditions.size() == 1)
            || !d->conditions.isEmpty();
}

QPersistenceSqlCondition::QPersistenceSqlCondition(const QPersistenceSqlCondition &rhs) :
    d(rhs.d)
{
}

QPersistenceSqlCondition &QPersistenceSqlCondition::operator=(const QPersistenceSqlCondition &rhs)
{
    if (this != &rhs)
        d.operator=(rhs.d);

    return *this;
}

QPersistenceSqlCondition::~QPersistenceSqlCondition()
{
}

QPersistenceSqlCondition QPersistenceSqlCondition::operator !()
{
    QPersistenceSqlCondition result(QPersistenceSqlCondition::Not, QList<QPersistenceSqlCondition>() << *this);
    return result;
}

QPersistenceSqlCondition QPersistenceSqlCondition::operator ||(const QPersistenceSqlCondition &rhs)
{
    return QPersistenceSqlCondition(QPersistenceSqlCondition::Or, QList<QPersistenceSqlCondition>() << *this << rhs);
}

QPersistenceSqlCondition QPersistenceSqlCondition::operator &&(const QPersistenceSqlCondition &rhs)
{
    return QPersistenceSqlCondition(QPersistenceSqlCondition::And, QList<QPersistenceSqlCondition>() << *this << rhs);
}

QString QPersistenceSqlCondition::toWhereClause(bool bindValues) const
{
    if(d->booleanOperator == Not) {
        Q_ASSERT(d->conditions.size() == 1);

        return booleanOperator().append(d->conditions.first().toWhereClause());
    }

    if(!d->conditions.isEmpty()) {
        QStringList conditions;
        foreach(const QPersistenceSqlCondition &condition, d->conditions) {
            conditions.append(condition.toWhereClause(bindValues));
        }

        QString result = conditions.join(booleanOperator());
        if(d->conditions.size() > 1)
            result = result.prepend('(').append(')');

        return result;
    }

    Q_ASSERT(!d->key.isEmpty());

    return comparisonOperator().prepend(QString("\"%1\"").arg(d->key)).append("?");
}

QVariantList QPersistenceSqlCondition::bindValues() const
{
    QVariantList result;

    foreach(const QPersistenceSqlCondition& condition, d->conditions) {
        result.append(condition.bindValues());
    }

    if(!d->key.isEmpty())
        result.append(d->value);

    return result;
}

QString QPersistenceSqlCondition::booleanOperator() const
{
    switch(d->booleanOperator) {
    case And:
        return " AND ";
    case Or:
        return " OR ";
    case Not:
        return "NOT ";
    }
}

QString QPersistenceSqlCondition::comparisonOperator() const
{
    switch(d->comparisonOperator) {
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


