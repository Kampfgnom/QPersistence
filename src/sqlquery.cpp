#include "sqlquery.h"

#include "qpersistence.h"
#include "sqlcondition.h"

#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QPixmap>
#include <QRegularExpressionMatchIterator>
#include <QSharedData>
#include <QStringList>
#define COMMA ,

class QpSqlQueryPrivate : public QSharedData {
public:
    QpSqlQueryPrivate() :
        QSharedData(),
        count(-1),
        skip(-1),
        canBulkExec(false)
    {}

    QString table;
    QHash<QString, QVariant> fields;
    int count;
    int skip;
    QpSqlCondition whereCondition;
    QList<QPair<QString, QpSqlQuery::Order> > orderBy;
    QList<QStringList> foreignKeys;
    bool canBulkExec;

    static bool debugEnabled;
    static QList<QpSqlQuery> bulkQueries;
    static bool bulkExec;
};

bool QpSqlQueryPrivate::bulkExec = false;
bool QpSqlQueryPrivate::debugEnabled = false;
QList<QpSqlQuery> QpSqlQueryPrivate::bulkQueries;

QpSqlQuery::QpSqlQuery() :
    QSqlQuery(),
    data(new QpSqlQueryPrivate)
{
}

QpSqlQuery::QpSqlQuery(const QSqlDatabase &database) :
    QSqlQuery(database),
    data(new QpSqlQueryPrivate)
{
}

QpSqlQuery::QpSqlQuery(const QpSqlQuery &rhs) :
    QSqlQuery(rhs),
    data(rhs.data)
{
}

QpSqlQuery &QpSqlQuery::operator=(const QpSqlQuery &rhs)
{
    QSqlQuery::operator =(rhs);

    if (this != &rhs)
        data.operator=(rhs.data);

    return *this;
}

QpSqlQuery::~QpSqlQuery()
{
}

bool QpSqlQuery::exec(const QString &queryString)
{
    bool ok = true;
    QString query = queryString;
    if (query.isEmpty()) {
        if (QpSqlQueryPrivate::bulkExec
                && data->canBulkExec) {
            QpSqlQueryPrivate::bulkQueries.append(*this);
        }
        else {
            ok = QSqlQuery::exec();
        }
    }
    else {
        ok = QSqlQuery::exec(queryString);
    }

    if (!ok || data->debugEnabled) {
        query = executedQuery();
        int index = query.indexOf('?');
        int i = 0;
        QList<QVariant> values = boundValues().values();
        while (index >= 0) {
            QString value = values.at(i).toString();
            if (value.isEmpty())
                value = QLatin1String("NULL");
            query.replace(index, 1, value);
            index = query.indexOf('?', index + value.length());
            ++i;
        }
        qDebug() << qPrintable(query);
    }

    return ok;
}

bool QpSqlQuery::exec()
{
    return exec(QString());
}

QString QpSqlQuery::table() const
{
    return data->table;
}

void QpSqlQuery::clear()
{
    QSqlQuery::clear();

    data->table = QString();
    data->fields.clear();
    data->count = -1;
    data->skip = -1;
    data->whereCondition = QpSqlCondition();
    data->orderBy.clear();
    data->foreignKeys.clear();
}

bool QpSqlQuery::isDebugEnabled()
{
    return QpSqlQueryPrivate::debugEnabled;
}

void QpSqlQuery::setDebugEnabled(bool value)
{
    QpSqlQueryPrivate::debugEnabled = value;
}

void QpSqlQuery::startBulkExec()
{
    QpSqlQueryPrivate::bulkExec = true;
}


void QpSqlQuery::setTable(const QString &table)
{
    data->table = table;
}

void QpSqlQuery::addField(const QString &name, const QVariant &value)
{
    data->fields.insert(name, value);
}

void QpSqlQuery::addForeignKey(const QString &columnName, const QString &keyName, const QString &foreignTableName)
{
    data->foreignKeys.append(QStringList() << columnName << keyName << foreignTableName);
}

void QpSqlQuery::setCount(int count)
{
    data->count = count;
}

void QpSqlQuery::setSkip(int skip)
{
    data->skip = skip;
}

void QpSqlQuery::setWhereCondition(const QpSqlCondition &condition)
{
    data->whereCondition = condition;
}

void QpSqlQuery::addOrder(const QString &field, QpSqlQuery::Order order)
{
    data->orderBy.append(QPair<QString, QpSqlQuery::Order>(field, order));
}

void QpSqlQuery::prepareCreateTable()
{
    QString query("CREATE TABLE \"");
    query.append(data->table).append("\" (\n\t");

    QStringList fields;
    QHashIterator<QString, QVariant> it(data->fields);
    while (it.hasNext()) {
        it.next();
        fields.append(QString("\"%1\" %2")
                      .arg(it.key())
                      .arg(it.value().toString()));
    }
    query.append(fields.join(",\n\t"));

    foreach (const QStringList foreignKey, data->foreignKeys) {
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
    query.append(data->table).append("\";");

    QSqlQuery::prepare(query);
}

void QpSqlQuery::prepareAlterTable()
{
    QSqlQuery::prepare(QString("ALTER TABLE \"%1\" ADD COLUMN \"%2\" %3;")
                       .arg(data->table)
                       .arg(data->fields.keys().first())
                       .arg(data->fields.values().first().toString()));
}

void QpSqlQuery::prepareSelect()
{
    QString query("SELECT ");

    if (data->fields.isEmpty()) {
        query.append("*");
    }
    else {
        QStringList fields;
        foreach (const QString &field, data->fields.keys()) {
            fields.append(QString("\"%1\"").arg(field));
        }
        query.append(fields.join(", "));
    }

    query.append(" FROM \"").append(data->table).append("\"");

    if (data->whereCondition.isValid()) {
        query.append("\n\tWHERE ").append(data->whereCondition.toWhereClause());
    }

    if (!data->orderBy.isEmpty()) {
        query.append("\n\tORDER BY ");
        QStringList orderClauses;
        foreach (QPair<QString COMMA QpSqlQuery::Order> order, data->orderBy) {
            QString orderClause = order.first.prepend("\n\t\t");
            if (order.second == QpSqlQuery::Descending)
                orderClause.append(" DESC");
            else
                orderClause.append(" ASC");
            orderClauses.append(orderClause);
        }
        query.append(orderClauses.join(','));
    }

    if (data->count >= 0) {
        query.append(QString("\n\tLIMIT "));
        if (data->skip >= 0) {
            query.append(QString("%1, ").arg(data->skip));
        }
        query.append(QString("%1").arg(data->count));
    }

    query.append(';');
    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }
}

bool QpSqlQuery::prepareUpdate()
{
    if (data->fields.isEmpty())
        return false;

    QString query("UPDATE \"");
    query.append(data->table).append("\" SET\n\t");

    QStringList fields;
    foreach (const QString &field, data->fields.keys()) {
        fields.append(QString("\"%1\" = ?").arg(field));
    }
    query.append(fields.join(",\n\t"));

    if (data->whereCondition.isValid()) {
        query.append("\n\tWHERE ").append(data->whereCondition.toWhereClause());
    }

    query.append(';');
    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->fields.values()) {
        addBindValue(value);
    }
    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }

    data->canBulkExec = true;
    return true;
}

void QpSqlQuery::prepareInsert()
{
    QString query("INSERT INTO \"");
    query.append(data->table).append("\"\n\t(");

    QStringList fields;
    foreach (const QString &field, data->fields.keys()) {
        fields.append(QString("\"%1\"").arg(field));
    }
    query.append(fields.join(", "));

    query.append(")\n\tVALUES ");
    query.append("(");
    query.append(QString("?, ").repeated(fields.size() - 1));
    if (fields.size() > 0)
        query.append("?");
    query.append("),");

    // Remove the last comma
    query.remove(query.length() - 1, 1);

    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->fields.values()) {
        addBindValue(value);
    }
}

void QpSqlQuery::prepareDelete()
{
    QString query("DELETE FROM \"");
    query.append(data->table).append("\"\n\tWHERE ");

    if (data->whereCondition.isValid()) {
        query.append(data->whereCondition.toWhereClause());
    }

    query.append(';');
    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }
    data->canBulkExec = true;
}

void QpSqlQuery::addBindValue(const QVariant &val)
{
    QSqlQuery::addBindValue(variantToSqlStorableVariant(val));
}

QVariant QpSqlQuery::variantToSqlStorableVariant(const QVariant &val)
{
    QVariant value = val;
    if (static_cast<QMetaType::Type>(val.type()) == QMetaType::QStringList) {
        return QVariant::fromValue<QString>(val.toStringList().join(','));
    }
    else if (static_cast<QMetaType::Type>(val.type()) == QMetaType::QPixmap) {
        QByteArray byteArray;
        QPixmap pixmap = val.value<QPixmap>();

        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");
        return byteArray.toBase64();
    }
    else if (Qp::Private::canConvertToSqlStorableVariant(val)) {
        return Qp::Private::convertToSqlStorableVariant(val);
    }

    return value;
}

QVariant QpSqlQuery::variantFromSqlStorableVariant(const QVariant &val, QMetaType::Type type)
{
    QVariant value = val;
    if (type == QMetaType::QStringList) {
        return QVariant::fromValue<QStringList>(val.toString().split(','));
    }
    else if (type == QMetaType::QPixmap) {
        QByteArray byteArray = QByteArray::fromBase64(val.toByteArray());
        QPixmap pixmap;
        pixmap.loadFromData(byteArray, "PNG");
        return QVariant::fromValue<QPixmap>(pixmap);
    }
    else if (Qp::Private::canConvertFromSqlStoredVariant(type)) {
        return Qp::Private::convertFromSqlStoredVariant(val.toString(), type);
    }
    return value;
}

void QpSqlQuery::bulkExec()
{
    Qp::database().transaction();
    QpSqlQueryPrivate::bulkExec = false;
    foreach (QpSqlQuery query, QpSqlQueryPrivate::bulkQueries) {
        query.exec();
    }
    Qp::database().commit();

    QpSqlQueryPrivate::bulkQueries.clear();
    QpSqlQueryPrivate::bulkExec = false;
}


#undef COMMA
