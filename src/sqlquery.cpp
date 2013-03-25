#include "sqlquery.h"

#include "sqlcondition.h"
#include "qpersistence.h"

#include <QSharedData>
#include <QStringList>
#include <QHash>
#include <QDebug>
#include <QRegularExpressionMatchIterator>
#define COMMA ,



class QPersistenceSqlQueryPrivate : public QSharedData {
public:
    QPersistenceSqlQueryPrivate() :
        QSharedData(),
        limit(-1)
    {}

    QString table;
    QHash<QString, QVariant> fields;
    int limit;
    QPersistenceSqlCondition whereCondition;
    QList<QPair<QString, QPersistenceSqlQuery::Order> > orderBy;
    QList<QStringList> foreignKeys;
};

QPersistenceSqlQuery::QPersistenceSqlQuery() :
    QSqlQuery(),
    d(new QPersistenceSqlQueryPrivate)
{
}

QPersistenceSqlQuery::QPersistenceSqlQuery(const QSqlDatabase &database) :
    QSqlQuery(database),
    d(new QPersistenceSqlQueryPrivate)
{
}

QPersistenceSqlQuery::QPersistenceSqlQuery(const QPersistenceSqlQuery &rhs) :
    QSqlQuery(rhs),
    d(rhs.d)
{
}

QPersistenceSqlQuery &QPersistenceSqlQuery::operator=(const QPersistenceSqlQuery &rhs)
{
    QSqlQuery::operator =(rhs);

    if (this != &rhs)
        d.operator=(rhs.d);

    return *this;
}

QPersistenceSqlQuery::~QPersistenceSqlQuery()
{
}

bool QPersistenceSqlQuery::exec()
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

QString QPersistenceSqlQuery::table() const
{
    return d->table;
}

void QPersistenceSqlQuery::clear()
{
    QSqlQuery::clear();

    d->table = QString();
    d->fields.clear();
    d->limit = -1;
    d->whereCondition = QPersistenceSqlCondition();
    d->orderBy.clear();
    d->foreignKeys.clear();
}

void QPersistenceSqlQuery::setTable(const QString &table)
{
    d->table = table;
}

void QPersistenceSqlQuery::addField(const QString &name, const QVariant &value)
{
    d->fields.insert(name, value);
}

void QPersistenceSqlQuery::addForeignKey(const QString &columnName, const QString &keyName, const QString &foreignTableName)
{
    d->foreignKeys.append(QStringList() << columnName << keyName << foreignTableName);
}

void QPersistenceSqlQuery::setLimit(int limit)
{
    d->limit = limit;
}

void QPersistenceSqlQuery::setWhereCondition(const QPersistenceSqlCondition &condition)
{
    d->whereCondition = condition;
}

void QPersistenceSqlQuery::addOrder(const QString &field, QPersistenceSqlQuery::Order order)
{
    d->orderBy.append(QPair<QString, QPersistenceSqlQuery::Order>(field, order));
}

void QPersistenceSqlQuery::prepareCreateTable()
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

void QPersistenceSqlQuery::prepareDropTable()
{
    QString query("DROP TABLE \"");
    query.append(d->table).append("\";");

    QSqlQuery::prepare(query);
}

void QPersistenceSqlQuery::prepareAlterTable()
{
    QSqlQuery::prepare(QString("ALTER TABLE \"%1\" ADD COLUMN \"%2\" %3;")
                       .arg(d->table)
                       .arg(d->fields.keys().first())
                       .arg(d->fields.values().first().toString()));
}

void QPersistenceSqlQuery::prepareSelect()
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
        foreach(QPair<QString COMMA QPersistenceSqlQuery::Order> order, d->orderBy) {
            QString orderClause = order.first.prepend("\n\t\t");
            if(order.second == QPersistenceSqlQuery::Descending)
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

bool QPersistenceSqlQuery::prepareUpdate()
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

void QPersistenceSqlQuery::prepareInsert()
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

void QPersistenceSqlQuery::prepareDelete()
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

void QPersistenceSqlQuery::addBindValue(const QVariant &val)
{
    QSqlQuery::addBindValue(variantToSqlStorableVariant(val));
}

QVariant QPersistenceSqlQuery::variantToSqlStorableVariant(const QVariant &val)
{
    QVariant value = val;
    if(static_cast<QMetaType::Type>(val.type()) == QMetaType::QStringList) {
        value = QVariant::fromValue<QString>(val.toStringList().join(','));
    }
    else if(QPersistence::Private::canConvertToSqlStorableVariant(val)) {
        value = QPersistence::Private::convertToSqlStorableVariant(val);
    }

    return value;
}

QVariant QPersistenceSqlQuery::variantFromSqlStorableVariant(const QVariant &val, QMetaType::Type type)
{
    QVariant value = val;
    if(type == QMetaType::QStringList) {
        value = QVariant::fromValue<QStringList>(val.toString().split(','));
    }
    else if(QPersistence::Private::canConvertFromSqlStoredVariant(type)) {
        value = QPersistence::Private::convertFromSqlStoredVariant(val.toString(), type);
    }
    return value;
}



#undef COMMA
