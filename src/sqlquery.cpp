#include "sqlquery.h"

#include "sqlcondition.h"
#include "qpersistence.h"

#include <QSharedData>
#include <QStringList>
#include <QHash>
#include <QDebug>
#include <QRegularExpressionMatchIterator>
#define COMMA ,



class QpSqlQueryPrivate : public QSharedData {
public:
    QpSqlQueryPrivate() :
        QSharedData(),
        limit(-1)
    {}

    QString table;
    QHash<QString, QVariant> fields;
    int limit;
    QpSqlCondition whereCondition;
    QList<QPair<QString, QpSqlQuery::Order> > orderBy;
    QList<QStringList> foreignKeys;
};

QpSqlQuery::QpSqlQuery() :
    QSqlQuery(),
    d(new QpSqlQueryPrivate)
{
}

QpSqlQuery::QpSqlQuery(const QSqlDatabase &database) :
    QSqlQuery(database),
    d(new QpSqlQueryPrivate)
{
}

QpSqlQuery::QpSqlQuery(const QpSqlQuery &rhs) :
    QSqlQuery(rhs),
    d(rhs.d)
{
}

QpSqlQuery &QpSqlQuery::operator=(const QpSqlQuery &rhs)
{
    QSqlQuery::operator =(rhs);

    if (this != &rhs)
        d.operator=(rhs.d);

    return *this;
}

QpSqlQuery::~QpSqlQuery()
{
}

bool QpSqlQuery::exec()
{
    bool ok = QSqlQuery::exec();
    QString query = executedQuery();
    int index = query.indexOf('?');
    int i = 0;
    QList<QVariant> values = boundValues().values();
    while (index >= 0) {
        QString value = values.at(i).toString();
        if(value.isEmpty())
            value = QLatin1String("NULL");
        query.replace(index, 1, value);
        index = query.indexOf('?', index + value.length());
        ++i;
    }
    qDebug() << qPrintable(query);
    return ok;
}

QString QpSqlQuery::table() const
{
    return d->table;
}

void QpSqlQuery::clear()
{
    QSqlQuery::clear();

    d->table = QString();
    d->fields.clear();
    d->limit = -1;
    d->whereCondition = QpSqlCondition();
    d->orderBy.clear();
    d->foreignKeys.clear();
}

void QpSqlQuery::setTable(const QString &table)
{
    d->table = table;
}

void QpSqlQuery::addField(const QString &name, const QVariant &value)
{
    d->fields.insert(name, value);
}

void QpSqlQuery::addForeignKey(const QString &columnName, const QString &keyName, const QString &foreignTableName)
{
    d->foreignKeys.append(QStringList() << columnName << keyName << foreignTableName);
}

void QpSqlQuery::setLimit(int limit)
{
    d->limit = limit;
}

void QpSqlQuery::setWhereCondition(const QpSqlCondition &condition)
{
    d->whereCondition = condition;
}

void QpSqlQuery::addOrder(const QString &field, QpSqlQuery::Order order)
{
    d->orderBy.append(QPair<QString, QpSqlQuery::Order>(field, order));
}

void QpSqlQuery::prepareCreateTable()
{
    QString query("CREATE TABLE \"");
    query.append(d->table).append("\" (\n\t");

    QStringList fields;
    QHashIterator<QString, QVariant> it(d->fields);
    while(it.hasNext()) {
        it.next();
        fields.append(QString("\"%1\" %2")
                      .arg(it.key())
                      .arg(it.value().toString()));
    }
    query.append(fields.join(",\n\t"));

    foreach(const QStringList foreignKey, d->foreignKeys) {
        query.append(QString(",\n\tFOREIGN KEY (%1) REFERENCES %2(%3)")
                     .arg(foreignKey.first())
                     .arg(foreignKey.last())
                     .arg(foreignKey.at(1)));
    }

    query.append("\n);");

    QSqlQuery::prepare(query);
}

void QpSqlQuery::prepareDropTable()
{
    QString query("DROP TABLE \"");
    query.append(d->table).append("\";");

    QSqlQuery::prepare(query);
}

void QpSqlQuery::prepareAlterTable()
{
    QSqlQuery::prepare(QString("ALTER TABLE \"%1\" ADD COLUMN \"%2\" %3;")
                       .arg(d->table)
                       .arg(d->fields.keys().first())
                       .arg(d->fields.values().first().toString()));
}

void QpSqlQuery::prepareSelect()
{
    QString query("SELECT ");

    if(d->fields.isEmpty()) {
        query.append("*");
    }
    else {
        QStringList fields;
        foreach(const QString &field, d->fields.keys()) {
            fields.append(QString("\"%1\"").arg(field));
        }
        query.append(fields.join(", "));
    }

    query.append(" FROM \"").append(d->table).append("\"");

    if(d->whereCondition.isValid()) {
        query.append("\n\tWHERE ").append(d->whereCondition.toWhereClause());
    }

    if(!d->orderBy.isEmpty()) {
        query.append("\n\tORDER BY ");
        QStringList orderClauses;
        foreach(QPair<QString COMMA QpSqlQuery::Order> order, d->orderBy) {
            QString orderClause = order.first.prepend("\n\t\t");
            if(order.second == QpSqlQuery::Descending)
                orderClause.append(" DESC");
            else
                orderClause.append(" ASC");
            orderClauses.append(orderClause);
        }
        query.append(orderClauses.join(','));
    }

    if(d->limit >= 0) {
        query.append(QString("\n\tLIMIT %1").arg(d->limit));
    }

    query.append(';');
    QSqlQuery::prepare(query);

    foreach(const QVariant value, d->whereCondition.bindValues()) {
        addBindValue(value);
    }
}

bool QpSqlQuery::prepareUpdate()
{
    if(d->fields.isEmpty())
        return false;

    QString query("UPDATE \"");
    query.append(d->table).append("\" SET\n\t");

    QStringList fields;
    foreach(const QString &field, d->fields.keys()) {
        fields.append(QString("\"%1\" = ?").arg(field));
    }
    query.append(fields.join(",\n\t"));

    if(d->whereCondition.isValid()) {
        query.append("\n\tWHERE ").append(d->whereCondition.toWhereClause());
    }

    query.append(';');
    QSqlQuery::prepare(query);

    foreach(const QVariant value, d->fields.values()) {
        addBindValue(value);
    }
    foreach(const QVariant value, d->whereCondition.bindValues()) {
        addBindValue(value);
    }

    return true;
}

void QpSqlQuery::prepareInsert()
{
    QString query("INSERT INTO \"");
    query.append(d->table).append("\"\n\t(");

    QStringList fields;
    foreach(const QString &field, d->fields.keys()) {
        fields.append(QString("\"%1\"").arg(field));
    }
    query.append(fields.join(", "));

    query.append(")\n\tVALUES (");
    query.append(QString("?, ").repeated(fields.size() - 1));
    if(fields.size() > 0)
        query.append("?");

    query.append(");");

    QSqlQuery::prepare(query);

    foreach(const QVariant value, d->fields.values()) {
        addBindValue(value);
    }
}

void QpSqlQuery::prepareDelete()
{
    QString query("DELETE FROM \"");
    query.append(d->table).append("\"\n\tWHERE ");

    if(d->whereCondition.isValid()) {
        query.append(d->whereCondition.toWhereClause());
    }

    query.append(';');
    QSqlQuery::prepare(query);

    foreach(const QVariant value, d->whereCondition.bindValues()) {
        addBindValue(value);
    }
}

void QpSqlQuery::addBindValue(const QVariant &val)
{
    QSqlQuery::addBindValue(variantToSqlStorableVariant(val));
}

QVariant QpSqlQuery::variantToSqlStorableVariant(const QVariant &val)
{
    QVariant value = val;
    if(static_cast<QMetaType::Type>(val.type()) == QMetaType::QStringList) {
        value = QVariant::fromValue<QString>(val.toStringList().join(','));
    }
    else if(Qp::Private::canConvertToSqlStorableVariant(val)) {
        value = Qp::Private::convertToSqlStorableVariant(val);
    }

    return value;
}

QVariant QpSqlQuery::variantFromSqlStorableVariant(const QVariant &val, QMetaType::Type type)
{
    QVariant value = val;
    if(type == QMetaType::QStringList) {
        value = QVariant::fromValue<QStringList>(val.toString().split(','));
    }
    else if(Qp::Private::canConvertFromSqlStoredVariant(type)) {
        value = Qp::Private::convertFromSqlStoredVariant(val.toString(), type);
    }
    return value;
}



#undef COMMA
