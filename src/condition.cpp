#include "condition.h"

#include "databaseschema.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QList>
#include <QSharedData>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


/******************************************************************************
 * QpConditionData
 */
class QpConditionData : public QSharedData {
public:
    QpConditionData() :
        QSharedData(),
        bindValues(false)
    {
    }
    bool bindValues;
    QString rawString;
    QString field;
    QVariant value;
    QpCondition::BooleanOperator booleanOperator;
    QpCondition::ComparisonOperator comparisonOperator;
    QList<QpCondition> conditions;
    QString table;
};


/******************************************************************************
 * QpCondition
 */
QpCondition QpCondition::notDeletedAnd(const QpCondition &additionalConditions)
{
    QpCondition notDeleted = QpCondition(QpDatabaseSchema::COLUMN_NAME_DELETEDFLAG, NotEqualTo, "1");
    if (!additionalConditions.isValid())
        return notDeleted;

    QpCondition cond = QpCondition(And, QList<QpCondition>()
                                         << notDeleted
                                         << additionalConditions);
    cond.setBindValuesAsString(additionalConditions.data->bindValues);
    return cond;
}

QpCondition::QpCondition() :
    data(new QpConditionData)
{
}

QpCondition::QpCondition(const QString &rawString) :
    data(new QpConditionData)
{
    data->rawString = rawString;
}

QpCondition::QpCondition(const QString &field, QpCondition::ComparisonOperator op, const QVariant &value) :
    data(new QpConditionData)
{
    data->field = field;
    data->booleanOperator = And;
    data->comparisonOperator = op;
    data->value = value;
}

QpCondition::QpCondition(QpCondition::BooleanOperator op, QpCondition &condition) :
    data(new QpConditionData)
{
    data->booleanOperator = op;
    data->conditions << condition;
    data->comparisonOperator = EqualTo;
}

QpCondition::QpCondition(QpCondition::BooleanOperator op, const QList<QpCondition> &conditions) :
    data(new QpConditionData)
{
    data->booleanOperator = op;
    data->conditions = conditions;
    data->comparisonOperator = EqualTo;
}

bool QpCondition::isValid() const
{
    return !data->field.isEmpty()
           || !data->rawString.isEmpty()
           || (data->booleanOperator == Not
               && data->conditions.size() == 1)
           || !data->conditions.isEmpty();
}

void QpCondition::setBindValuesAsString(bool bindValues)
{
    data->bindValues = bindValues;

    for (int i = 0; i < data->conditions.size(); ++i) {
        data->conditions[i].setBindValuesAsString(bindValues);
    }
}

bool QpCondition::bindValuesAsString() const
{
    return data->bindValues;
}

QpCondition::QpCondition(const QpCondition &rhs) :
    data(rhs.data)
{
}

QpCondition &QpCondition::operator=(const QpCondition &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);

    return *this;
}

QpCondition::~QpCondition()
{
}

QpCondition QpCondition::operator !()
{
    QpCondition result(QpCondition::Not, QList<QpCondition>() << *this);
    return result;
}

QpCondition QpCondition::operator ||(const QpCondition &rhs)
{
    if (!isValid())
        return rhs;

    if (!rhs.isValid())
        return *this;

    return QpCondition(QpCondition::Or, QList<QpCondition>() << *this << rhs);
}

QpCondition QpCondition::operator &&(const QpCondition &rhs)
{
    if (!isValid())
        return rhs;

    if (!rhs.isValid())
        return *this;

    return QpCondition(QpCondition::And, QList<QpCondition>() << *this << rhs);
}

QString QpCondition::toSqlClause() const
{
    if (!data->rawString.isEmpty())
        return data->rawString;

    if (data->booleanOperator == Not) {
        Q_ASSERT(data->conditions.size() == 1);

        return booleanOperatorSqlString().append(data->conditions.first().toSqlClause());
    }

    if (!data->conditions.isEmpty()) {
        QStringList conditions;
        foreach (const QpCondition &condition, data->conditions) {
            conditions.append(condition.toSqlClause());
        }

        QString result = conditions.join(booleanOperatorSqlString());
        if (data->conditions.size() > 1)
            result = result.prepend('(').append(')');

        return result;
    }

    Q_ASSERT(!data->field.isEmpty());

    QString value = "?";
    if (data->bindValues)
        value = data->value.toString();

    if (!data->table.isEmpty()
        && !data->field.contains('.')) {
        return comparisonOperatorSqlString()
               .prepend(QString("%1.%2")
                        .arg(QpSqlQuery::escapeField(data->table))
                        .arg(QpSqlQuery::escapeField(data->field)))
               .append(value);
    }

    return comparisonOperatorSqlString()
           .prepend(QpSqlQuery::escapeField(data->field))
           .append(value);
}

QVariantList QpCondition::bindValues() const
{
    QVariantList result;

    if (data->bindValues)
        return result;

    if (!data->rawString.isEmpty())
        return result;

    foreach (const QpCondition& condition, data->conditions) {
        result.append(condition.bindValues());
    }

    if (!data->field.isEmpty())
        result.append(data->value);

    return result;
}

QpCondition::BooleanOperator QpCondition::booleanOperator() const
{
    return data->booleanOperator;
}

QString QpCondition::booleanOperatorSqlString() const
{
    switch (data->booleanOperator) {
    case And:
        return " AND ";
    case Or:
        return " OR ";
    case Not:
        return "NOT ";
    }

    Q_ASSERT(false);
    return QString();
}

QpCondition::ComparisonOperator QpCondition::comparisonOperator() const
{
    return data->comparisonOperator;
}

QString QpCondition::comparisonOperatorSqlString() const
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

    Q_ASSERT(false);
    return QString();
}

void QpCondition::setTable(const QString &table)
{
    QList<QpCondition> newData;
    foreach (QpCondition c, data->conditions) {
        c.setTable(table);
        newData << c;
    }

    data->conditions = newData;
    data->table = table;
}
