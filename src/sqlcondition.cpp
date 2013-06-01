#include "sqlcondition.h"

#include <QSharedData>
#include <QList>
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
    d(new QpSqlConditionData)
{
}

QpSqlCondition::QpSqlCondition(const QString &rawString) :
    d(new QpSqlConditionData)
{
    d->rawString = rawString;
}

QpSqlCondition::QpSqlCondition(const QString &key, QpSqlCondition::ComparisonOperator op, const QVariant &value) :
    d(new QpSqlConditionData)
{
    d->key = key;
    d->booleanOperator = And;
    d->comparisonOperator = op;
    d->value = value;
}

QpSqlCondition::QpSqlCondition(QpSqlCondition::BooleanOperator op, const QList<QpSqlCondition> &conditions) :
    d(new QpSqlConditionData)
{
    d->booleanOperator = op;
    d->conditions = conditions;
    d->comparisonOperator = EqualTo;
}

bool QpSqlCondition::isValid() const
{
    return !d->key.isEmpty()
            || (d->booleanOperator == Not
                && d->conditions.size() == 1)
            || !d->conditions.isEmpty();
}

QpSqlCondition::QpSqlCondition(const QpSqlCondition &rhs) :
    d(rhs.d)
{
}

QpSqlCondition &QpSqlCondition::operator=(const QpSqlCondition &rhs)
{
    if (this != &rhs)
        d.operator=(rhs.d);

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
    if(!d->rawString.isEmpty())
        return d->rawString;

    if(d->booleanOperator == Not) {
        Q_ASSERT(d->conditions.size() == 1);

        return booleanOperator().append(d->conditions.first().toWhereClause());
    }

    if(!d->conditions.isEmpty()) {
        QStringList conditions;
        foreach(const QpSqlCondition &condition, d->conditions) {
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

QVariantList QpSqlCondition::bindValues() const
{
    QVariantList result;

    foreach(const QpSqlCondition& condition, d->conditions) {
        result.append(condition.bindValues());
    }

    if(!d->key.isEmpty())
        result.append(d->value);

    return result;
}

QString QpSqlCondition::booleanOperator() const
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

QString QpSqlCondition::comparisonOperator() const
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


