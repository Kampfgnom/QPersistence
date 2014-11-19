#include "sqlquery.h"

#include "databaseschema.h"
#include "qpersistence.h"
#include "sqlcondition.h"
#include "sqlbackend.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QMetaProperty>
#include <QRegularExpressionMatchIterator>
#include <QSharedData>
#include <QSqlDriver>
#include <QSqlRecord>
#include <QStringList>
#ifndef QP_NO_GUI
#   include <QPixmap>
#endif
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpSqlQueryPrivate : public QSharedData {
public:
    QpSqlQueryPrivate() :
        QSharedData(),
        count(-1),
        skip(-1),
        ignore(false),
        forUpdate(false)
    {}

    struct Join {
        QString direction;
        QString table;
        QString on;
    };

    int count;
    QSqlDatabase database;
    QpSqlBackend *backend;
    QString table;
    QHash<QString, QVariant> fields;
    // inserted directly into query instead of using bindValue
    QHash<QString, QString> rawFields;
    QpSqlCondition whereCondition;
    QList<QPair<QString, QpSqlQuery::Order> > orderBy;
    QList<QStringList> foreignKeys;
    QHash<QString, QStringList> keys;
    QHash<int, int> propertyIndexes;
    QStringList groups;
    int skip;
    bool ignore;
    bool forUpdate;
    QList<Join> joins;

    static bool debugEnabled;

    QString constructSelectQuery() const;
    QString escapedQualifiedField(const QString &field) const;
};

bool QpSqlQueryPrivate::debugEnabled = false;

QpSqlQuery::QpSqlQuery() :
    QSqlQuery(),
    data(new QpSqlQueryPrivate)
{
}

QpSqlQuery::QpSqlQuery(const QSqlDatabase &database) :
    QSqlQuery(database),
    data(new QpSqlQueryPrivate)
{
    data->database = database;
    data->backend = QpSqlBackend::forDatabase(database);
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
        ok = QSqlQuery::exec();
    }
    else {
        ok = QSqlQuery::exec(queryString);
    }

    if (data->debugEnabled) {
        query = executedQuery();
        int index = query.indexOf('?');
        int i = 0;
        QList<QVariant> values = boundValues().values();
        while (index >= 0 && i < values.size()) {
            QString value = values.at(i).toString();
            if (value.isEmpty())
                value = QLatin1String("NULL");
            query.replace(index, 1, value);
            index = query.indexOf('?', index + value.length());
            ++i;
        }
        if(size() >= 0)
            query += QString("\n%1 rows returned.").arg(size());
        if(numRowsAffected() >= 0)
            query += QString("\n%1 rows affected.").arg(numRowsAffected());
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
    data->keys.clear();
    data->rawFields.clear();
    data->ignore = false;
    data->forUpdate = false;
    data->propertyIndexes.clear();
}

bool QpSqlQuery::isDebugEnabled()
{
    return QpSqlQueryPrivate::debugEnabled;
}

void QpSqlQuery::setDebugEnabled(bool value)
{
    QpSqlQueryPrivate::debugEnabled = value;
}

QString QpSqlQueryPrivate::escapedQualifiedField(const QString &field) const
{
    if(table.isEmpty() || field.contains('.'))
        return QpSqlQuery::escapeField(field);

    return QString("%1.%2")
            .arg(QpSqlQuery::escapeField(table))
            .arg(QpSqlQuery::escapeField(field));
}

QString QpSqlQuery::escapedQualifiedField(const QString &field) const
{
    return data->escapedQualifiedField(field);
}

QString QpSqlQuery::escapeField(const QString &field)
{
    if(field.startsWith('('))
        return field;

    QStringList fields = field.split(".");
    QStringList escaped;
    foreach(QString f, fields) {
        QStringList split = f.split('+');
        if(split.size() == 2)
            escaped << QString("%1+%2").arg(escapeField(split.at(0))).arg(split.at(1));
        else if(!f.contains('`'))
            escaped << QString("`%1`").arg(f);
        else
            escaped << f;
    }

    return escaped.join(".");
}

void QpSqlQuery::setTable(const QString &table)
{
    data->table = table;
}

void QpSqlQuery::addPrimaryKey(const QString &name)
{
    addField(name, data->backend->primaryKeyType());
}

void QpSqlQuery::addKey(const QString &keyType, const QStringList &fields)
{
    data->keys.insert(keyType, fields);
}

void QpSqlQuery::setOrIgnore(bool ignore)
{
    data->ignore = ignore;
}

void QpSqlQuery::addRawField(const QString &name, const QString &value)
{
    data->rawFields.insert(name, value);
}

void QpSqlQuery::addField(const QString &name, const QVariant &value)
{
    data->fields.insert(name, value);
}

void QpSqlQuery::addForeignKey(const QString &columnName,
                               const QString &keyName,
                               const QString &foreignTableName,
                               const QString &onDelete,
                               const QString &onUpdate)
{
    data->foreignKeys.append(QStringList() << columnName << keyName << foreignTableName << onDelete << onUpdate);
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
    data->whereCondition.setTable(data->table);
}

void QpSqlQuery::addOrder(const QString &field, QpSqlQuery::Order order)
{
    data->orderBy.append(QPair<QString, QpSqlQuery::Order>(field, order));
}

void QpSqlQuery::setForUpdate(bool forUpdate)
{
    data->forUpdate = forUpdate;
}

void QpSqlQuery::addJoin(const QString &direction, const QString &table, const QString &on)
{
    QpSqlQueryPrivate::Join join;
    join.direction = direction;
    join.table = table;
    join.on = on;
    data->joins.append(join);
}

void QpSqlQuery::addJoin(const QString &direction, const QpSqlQuery &subSelect, const QString &on)
{
    addJoin(direction,
            QString::fromLatin1("(%1) as sub_select_%2").arg(subSelect.data->constructSelectQuery()).arg(data->joins.size()),
            on);
    foreach(QVariant bindValue, subSelect.boundValues()) {
        addBindValue(bindValue);
    }
}

void QpSqlQuery::addGroupBy(const QString &groupBy)
{
    data->groups.append(groupBy);
}

void QpSqlQuery::prepareCreateTable()
{
    QString query("CREATE TABLE ");
    query.append(escapeField(data->table)).append(" (\n\t");

    QStringList fields;
    QHashIterator<QString, QVariant> it(data->fields);
    while (it.hasNext()) {
        it.next();
        fields.append(QString("%1 %2")
                      .arg(escapeField(it.key()))
                      .arg(it.value().toString()));
    }
    query.append(fields.join(",\n\t"));

    foreach(QString key, data->keys.keys()) {
        QStringList v = data->keys.value(key);
        QStringList values;

        QStringList::const_iterator it2;
        QStringList::const_iterator end = v.end();
        for(it2 = v.constBegin(); it2 != end; ++it2)
            values << escapeField(*it2);

        query.append(QString(",\n\t%1 ")
                     .arg(key));

#ifndef QP_FOR_SQLITE
        QString name = v.join("_").prepend("_Qp_key_");
        query.append(name);
#endif
        QString keyFields = values.join(", ");
        query.append(QString(" (%1)")
                     .arg(keyFields));
    }

    foreach (const QStringList foreignKey, data->foreignKeys) {
        query.append(QString(",\n\tFOREIGN KEY (%1) REFERENCES %2(%3)")
                     .arg(escapeField(foreignKey.first()))
                     .arg(escapeField(foreignKey.at(2)))
                     .arg(escapeField(foreignKey.at(1))));

        QString onDelete = foreignKey.at(3);
        if(!onDelete.isEmpty()) {
            query.append(QString("\n\t\tON DELETE %1").arg(onDelete));
        }
        QString onUpdate = foreignKey.at(4);
        if(!onUpdate.isEmpty()) {
            query.append(QString("\n\t\tON UPDATE %1").arg(onUpdate));
        }
    }

    query.append("\n);");

    QSqlQuery::prepare(query);
}

void QpSqlQuery::prepareDropTable()
{
    QString query("DROP TABLE ");
    query.append(escapeField(data->table)).append(";");

    QSqlQuery::prepare(query);
}

void QpSqlQuery::prepareAlterTable()
{
    QSqlQuery::prepare(QString("ALTER TABLE %1 ADD COLUMN %2 %3;")
                       .arg(escapeField(data->table))
                       .arg(escapeField(data->fields.keys().first()))
                       .arg(data->fields.values().first().toString()));
}

QString QpSqlQueryPrivate::constructSelectQuery() const
{
    QString query("SELECT ");

    if (fields.isEmpty() && rawFields.isEmpty()) {
        query.append("*");
    }
    else {
        QStringList localFields;
        foreach (const QString &field, fields.keys()) {
            localFields.append(QString("%1").arg(escapedQualifiedField(field)));
        }
        foreach(QString field, rawFields.keys()) {
            localFields.append(field);
        }

        query.append(localFields.join(", "));
    }

    query.append(" FROM ").append(QpSqlQuery::escapeField(table)).append("");

    foreach(QpSqlQueryPrivate::Join join, joins) {
        query.append(QString("\n%1 JOIN %2 ON %3")
                .arg(join.direction)
                .arg(QpSqlQuery::escapeField(join.table))
                .arg(join.on));
    }

    if (whereCondition.isValid()) {
        query.append("\n\tWHERE ").append(whereCondition.toWhereClause());
    }

    if(!groups.isEmpty()) {
        query.append("\n\tGROUP BY ");
        query.append(groups.join(','));
    }

    if (!orderBy.isEmpty()) {
        query.append("\n\tORDER BY ");
        QStringList orderClauses;
        typedef QPair<QString, QpSqlQuery::Order> OrderPair;
        foreach (OrderPair order, orderBy) {
            QString orderClause = escapedQualifiedField(order.first).prepend("\n\t\t");
            if (order.second == QpSqlQuery::Descending)
                orderClause.append(" DESC");
            else
                orderClause.append(" ASC");
            orderClauses.append(orderClause);
        }
        query.append(orderClauses.join(','));
    }

    if (count >= 0) {
        query.append(QString("\n\tLIMIT "));
        if (skip >= 0) {
            query.append(QString("%1, ").arg(skip));
        }
        query.append(QString("%1").arg(count));
    }

    if(forUpdate) {
        query.append(" FOR UPDATE ");
    }

    return query;
}

void QpSqlQuery::prepareSelect()
{
    QSqlQuery::prepare(data->constructSelectQuery());

    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }
}

bool QpSqlQuery::prepareUpdate()
{
    if (data->fields.isEmpty()
            && data->rawFields.isEmpty())
        return false;

    QString query("UPDATE ");
    query.append(escapeField(data->table)).append(" SET\n\t");

    QStringList fields;
    foreach (const QString &field, data->fields.keys()) {
        fields.append(QString("%1 = ?").arg(escapeField(field)));
    }
    foreach (const QString &field, data->rawFields.keys()) {
        fields.append(QString("%1 = %2")
                      .arg(escapeField(field))
                      .arg(data->rawFields.value(field)));
    }
    query.append(fields.join(",\n\t"));

    if (data->whereCondition.isValid()) {
        query.append("\n\tWHERE ").append(data->whereCondition.toWhereClause());
    }

    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->fields.values()) {
        addBindValue(value);
    }
    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }

    return true;
}

void QpSqlQuery::prepareInsert()
{
    QString query("INSERT ");
    if(data->ignore)
        query.append(data->backend->orIgnore());

    query.append(" INTO ");

    query.append(escapeField(data->table)).append("\n\t(");

    QStringList rawFieldKeys = data->rawFields.keys();

    QStringList fields;
    foreach (const QString &field, data->fields.keys()) {
        fields.append(QString("%1").arg(escapeField(field)));
    }
    foreach (QString field, rawFieldKeys) {
        fields.append(escapeField(field));
    }
    query.append(fields.join(", "));

    query.append(")\n\tVALUES (");

    fields = QStringList();
    int s = data->fields.size();
    for (int i = 0; i < s; ++i) {
        fields.append(QLatin1String("?"));
    }
    foreach (QString field, rawFieldKeys) {
        fields.append(data->rawFields.value(field));
    }

    query.append(fields.join(", "));
    query.append(") ");

    if (data->whereCondition.isValid()) {
        query.append("\n\tWHERE ").append(data->whereCondition.toWhereClause());
    }

    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->fields.values()) {
        addBindValue(value);
    }
    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }
}

void QpSqlQuery::prepareDelete()
{
    QString query("DELETE FROM ");
    query.append(escapeField(data->table));

    if (data->whereCondition.isValid()) {
        query.append("\n\tWHERE ");
        query.append(data->whereCondition.toWhereClause());
    }

    query.append(';');
    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }
}

void QpSqlQuery::prepareincrementNumericColumn()
{
    Q_ASSERT(data->fields.size() == 1);
    Q_ASSERT(data->whereCondition.isValid());

    QString query("UPDATE ");
    query.append(escapeField(data->table)).append(" SET\n\t");
    query.append(QString("%1 = (SELECT MAX(%1) + 1 FROM (SELECT %1 FROM %2) AS tempTable),")
                 .arg(escapeField(data->fields.keys().first()))
                 .arg(escapeField(data->table)));
    query.append(QString::fromLatin1("%1 = %2")
                 .arg(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME)
                 .arg(QpSqlBackend::forDatabase(data->database)->nowTimestamp()));
    query.append("\n\tWHERE ").append(data->whereCondition.toWhereClause());

    QSqlQuery::prepare(query);

    foreach (const QVariant value, data->whereCondition.bindValues()) {
        addBindValue(value);
    }
}

QMetaProperty QpSqlQuery::propertyForIndex(const QSqlRecord &record, const QMetaObject *metaObject, int index) const
{
    int propertyIndex = data->propertyIndexes.value(index, -123);
    if(propertyIndex > 0)
        return metaObject->property(propertyIndex);
    if(propertyIndex != -123)
        return QMetaProperty();

    QString fieldName = record.fieldName(index);
    propertyIndex = metaObject->indexOfProperty(fieldName.toLatin1());
    if(propertyIndex <= 0) {
        if(fieldName.contains('+'))
            fieldName = fieldName.left(fieldName.length() - 2);
        fieldName = fieldName.split('.').last();
        fieldName = fieldName.remove('`');
        propertyIndex = metaObject->indexOfProperty(fieldName.toLatin1()); // removes the "+0" from enum and flag fields
    }

    data->propertyIndexes.insert(index, propertyIndex);

    if(propertyIndex <= 0)
        return QMetaProperty();

    return metaObject->property(propertyIndex);
}

void QpSqlQuery::addBindValue(const QVariant &val)
{
    QSqlQuery::addBindValue(variantToSqlStorableVariant(val));
}

const char LISTSEPARATOR = 0x1;

QVariant QpSqlQuery::variantToSqlStorableVariant(const QVariant &val)
{
    QVariant value = val;

    if (val.type() == QVariant::DateTime) {
        QDateTime time = val.toDateTime();
        return time.toTimeSpec(Qt::UTC);
    }
    else if (val.type() == QVariant::Time) {
        QDateTime time = QDateTime(QDate::currentDate(), val.toTime());
        return time.toTimeSpec(Qt::UTC).time();
    }
    else if (static_cast<QMetaType::Type>(val.type()) == QMetaType::QStringList) {
        return QVariant::fromValue<QString>(val.toStringList().join(LISTSEPARATOR));
    }
    else if (static_cast<QMetaType::Type>(val.type()) == QMetaType::QPixmap) {
#ifndef QP_NO_GUI
        QByteArray byteArray;
        QPixmap pixmap = val.value<QPixmap>();

        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "png");
        return byteArray;
#endif
    }
    else if (Qp::Private::canConvertToSqlStorableVariant(val)) {
        return Qp::Private::convertToSqlStorableVariant(val);
    }

    return value;
}

QVariant QpSqlQuery::variantFromSqlStorableVariant(const QVariant &val, QMetaType::Type type)
{
    QVariant value = val;
    if (static_cast<QVariant::Type>(type) == QVariant::DateTime) {
        QDateTime time = val.toDateTime();
        time.setTimeSpec(Qt::UTC);
        return time.toLocalTime();
    }
    else if (static_cast<QVariant::Type>(type) == QVariant::Time) {
        QDateTime time = QDateTime(QDate::currentDate(), val.toTime());
        time.setTimeSpec(Qt::UTC);
        return time.toLocalTime();
    }
    if (type == QMetaType::QStringList) {
        QString string = val.toString();
        if(string.isEmpty())
            return QStringList();

        return QVariant::fromValue<QStringList>(string.split(LISTSEPARATOR));
    }
    else if (type == QMetaType::QPixmap) {
#ifndef QP_NO_GUI
        QByteArray byteArray = val.toByteArray();
        QPixmap pixmap;
        pixmap.loadFromData(byteArray, "png");
        return QVariant::fromValue<QPixmap>(pixmap);
#endif
    }
    else if (Qp::Private::canConvertFromSqlStoredVariant(type)) {
        return Qp::Private::convertFromSqlStoredVariant(val.toString(), type);
    }
    return value;
}

