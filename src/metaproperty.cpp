#include "metaproperty.h"

#include "metaobject.h"
#include "qpersistence.h"

#include <QDebug>
#include <QMetaClassInfo>
#include <QRegularExpression>
#include <QStringList>

static const QRegularExpression TOMANYRELATIONREGEXP("QList\\<(QSharedPointer|QWeakPointer)\\<(\\w+)\\> \\>");
static const QRegularExpression MAPPINGRELATIONREGEXP("QMap\\<(.+),(.+)\\>");
static const QRegularExpression SETTYPEREGEXP("QSet<(.+)\\>");

class QpMetaPropertyPrivate : public QSharedData
{
public:
    QpMetaPropertyPrivate() :
        QSharedData(),
        cardinality(QpMetaProperty::UnknownCardinality)
    {}

    QString typeName;
    QMetaProperty metaProperty;
    QpMetaObject metaObject;
    mutable QHash<QString, QString> attributes;
    QpMetaProperty::Cardinality cardinality;
    mutable QString columnName;
};

QpMetaProperty::QpMetaProperty() :
    data(new QpMetaPropertyPrivate)
{
}

QpMetaProperty::QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject) :
    data(new QpMetaPropertyPrivate)
{
    data->metaProperty = property;
    data->metaObject = metaObject;
    data->typeName = property.typeName();

    Q_ASSERT(isValid());
}

QString QpMetaProperty::generateColumnName() const
{
    if (!isRelationProperty())
        return QString(name());

    if (cardinality() == QpMetaProperty::ManyToManyCardinality) {
        return QString(data->metaObject.tableName()).prepend("_Qp_FK_");;
    }
    else if (isToManyRelationProperty()
             || (isToOneRelationProperty()
                 && !hasTableForeignKey())) {
        QpMetaProperty reverse(reverseRelation());
        return QString(reverse.name()).prepend("_Qp_FK_");
    }
    else if (isToOneRelationProperty()) {
        return QString(name()).prepend("_Qp_FK_");
    }

    return QString(name());
}

QString QpMetaProperty::shortName(const QString &name) const
{
    QString result = name;

    // TODO: find a better way to shorten the name
    // 56 has been chosen, because MySQL allows only 64 characters for many things including foreignkeys
    // Since its automatically generated names of foreign key contraints append 7 to 8 characters to the table name,
    // 56 is needed...
    // This simple truncating by using left() could lead to collisions, which are currently not being handled!
    if(result.size() > 56)
        result = result.left(56);

    return result;
}

QpMetaProperty::~QpMetaProperty()
{
}

QpMetaProperty::QpMetaProperty(const QpMetaProperty &other) :
    data(other.data)
{
}

QpMetaProperty &QpMetaProperty::operator =(const QpMetaProperty &other)
{
    if (&other != this)
        data.operator =(other.data);

    return *this;
}

QpMetaObject QpMetaProperty::metaObject() const
{
    return data->metaObject;
}

QMetaProperty QpMetaProperty::metaProperty() const
{
    return data->metaProperty;
}

QString QpMetaProperty::columnName() const
{
    if (data->columnName.isEmpty())
        data->columnName = shortName(generateColumnName());

    return data->columnName;
}

bool QpMetaProperty::isStored() const
{
    return data->metaProperty.isStored();
}

bool QpMetaProperty::isValid() const
{
    return data->metaProperty.isValid();
}

QVariant::Type QpMetaProperty::type() const
{
    return data->metaProperty.type();
}

bool QpMetaProperty::isRelationProperty() const
{
    return isToOneRelationProperty() || isToManyRelationProperty();
}

bool QpMetaProperty::isToOneRelationProperty() const
{
    if(data->cardinality == UnknownCardinality) {
        QString type(typeName());
        Q_ASSERT(!type.isEmpty());
        return type.startsWith("QSharedPointer<")
                || type.startsWith("QWeakPointer<");
    }

    return data->cardinality == OneToOneCardinality
            || data->cardinality == ManyToOneCardinality;
}

bool QpMetaProperty::isToManyRelationProperty() const
{
    if(data->cardinality == UnknownCardinality)
        return TOMANYRELATIONREGEXP.match(typeName()).hasMatch();

    return data->cardinality == OneToManyCardinality
            || data->cardinality == ManyToManyCardinality;
}

bool QpMetaProperty::hasTableForeignKey() const
{
    switch (cardinality()) {
    case QpMetaProperty::ManyToOneCardinality:
        return true;

    case QpMetaProperty::OneToManyCardinality:
        return false;

    case QpMetaProperty::OneToOneCardinality:
        return  metaObject().tableName() < reverseMetaObject().tableName();

    case QpMetaProperty::ManyToManyCardinality:
        return false;

    default:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    Q_ASSERT(false);
    return true;
}

QpMetaProperty::Cardinality QpMetaProperty::cardinality() const
{
    if (data->cardinality != UnknownCardinality)
        return data->cardinality;

    QString reverseName = reverseRelationName();
    Q_ASSERT(!reverseName.isEmpty());

    QpMetaProperty r = reverseRelation();
    if (isToOneRelationProperty()) {
        if (r.isToOneRelationProperty()) {
            data->cardinality = OneToOneCardinality;
        }
        else if (r.isToManyRelationProperty()) {
            data->cardinality = ManyToOneCardinality;
        }
    }
    else if (isToManyRelationProperty()) {
        if (r.isToManyRelationProperty()) {
            data->cardinality = ManyToManyCardinality;
        }
        else if (r.isToOneRelationProperty()) {
            data->cardinality = OneToManyCardinality;
        }
    }

    Q_ASSERT_X(data->cardinality != UnknownCardinality, Q_FUNC_INFO,
               QString("The relation %1 has no cardinality. This is an internal error and should never happen.")
               .arg(name())
               .toLatin1());

    return data->cardinality;
}

QString QpMetaProperty::reverseClassName() const
{
    QString name(typeName());
    if (isToOneRelationProperty()) {
        int l = name.length();
        return name.left(l - 1).right(l - 16);
    }

    QRegularExpressionMatch match = TOMANYRELATIONREGEXP.match(name);
    if (!match.hasMatch())
        return QString();

    return match.captured(2);
}

QpMetaObject QpMetaProperty::reverseMetaObject() const
{
    return QpMetaObject::forClassName(reverseClassName());
}

QString QpMetaProperty::reverseRelationName() const
{
    if (data->attributes.isEmpty()) {
        QString classInfoName = QString(QPERSISTENCE_PROPERTYMETADATA).append(":").append(name());
        QString classInfoRawValue = data->metaObject.classInformation(classInfoName.toLatin1(), QString());

        // First parse the attributes
        QRegularExpression reg("(\\w+)=(\\w+)");
        QStringList attributesList = classInfoRawValue.split(';');
        foreach (const QString attribute, attributesList) {
            QRegularExpressionMatch match = reg.match(attribute);
            if (!match.hasMatch())
                continue;

            data->attributes.insert( match.captured(1), match.captured(2) );
        }
    }

    return data->attributes.value(QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION);
}

QpMetaProperty QpMetaProperty::reverseRelation() const
{
    return reverseMetaObject().metaProperty(reverseRelationName());
}

QString QpMetaProperty::tableName() const
{
    if (!isRelationProperty())
        return metaObject().tableName();

    QString table = metaObject().tableName();
    QString reverseTable = reverseMetaObject().tableName();
    QString s1, s2;

    QString result;

    switch (cardinality()) {
    case QpMetaProperty::ManyToOneCardinality:
        // My table gets a foreign key column
        result = table;
        break;

    case QpMetaProperty::OneToManyCardinality:
        // The related table gets a foreign key column
        result = reverseTable;
        break;

    case QpMetaProperty::OneToOneCardinality:
        result = table < reverseTable ? table : reverseTable;
        break;

    case QpMetaProperty::ManyToManyCardinality:
        s1 = QString(name());
        s2 = QString(reverseRelation().name());
        if (table > reverseTable) {
            qSwap(table, reverseTable);
            qSwap(s1, s2);
        }
        result = QString("_Qp_REL_%1_%2__%3_%4")
                .arg(table)
                .arg(s1)
                .arg(reverseTable)
                .arg(s2);
        break;

    default:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    return shortName(result);
}

QMetaMethod QpMetaProperty::addObjectMethod()
{
    static QMetaObject mo = metaObject().metaObject();
    QString signature("add%1(QSharedPointer<%2>)");
    QString propertyName = name();
    propertyName[0] = propertyName.at(0).toTitleCase();
    signature = signature.arg(propertyName).arg(reverseClassName());
    int index = mo.indexOfMethod(QMetaObject::normalizedSignature(signature.toUtf8()));
    Q_ASSERT(index > 0);
    return mo.method(index);
}

QMetaMethod QpMetaProperty::removeObjectMethod()
{
    static QMetaObject mo = metaObject().metaObject();
    QString signature("remove%1(QSharedPointer<%2>)");
    QString propertyName = name();
    propertyName[0] = propertyName.at(0).toTitleCase();
    signature = signature.arg(propertyName).arg(reverseClassName());
    QByteArray normalized = QMetaObject::normalizedSignature(signature.toUtf8());
    int index = mo.indexOfMethod(normalized);

    Q_ASSERT(index > 0);
    return mo.method(index);
}

bool QpMetaProperty::isMappingProperty() const
{
    return QString(typeName()).startsWith("QMap");
}

QString QpMetaProperty::mappingFromTypeName() const
{
    QRegularExpressionMatch match = MAPPINGRELATIONREGEXP.match(typeName());
    if (!match.hasMatch())
        return QString();

    return match.captured(1);
}

QString QpMetaProperty::mappingToTypeName() const
{
    QRegularExpressionMatch match = MAPPINGRELATIONREGEXP.match(typeName());
    if (!match.hasMatch())
        return QString();

    return match.captured(2);
}

bool QpMetaProperty::isSetProperty() const
{
    return QString(typeName()).startsWith("QSet");
}

QString QpMetaProperty::setType() const
{
    QRegularExpressionMatch match = SETTYPEREGEXP.match(typeName());
    if (!match.hasMatch())
        return QString();

    return match.captured(2);
}

bool QpMetaProperty::write(QObject *obj, const QVariant &value) const
{
    if (!data->metaProperty.isWritable())
        return false;

    QVariant::Type t = data->metaProperty.type();
    if (value.canConvert(t)) {
        QVariant v(value);
        v.convert(t);
        return data->metaProperty.write( obj, v );
    }

    return data->metaProperty.write( obj, value );
}

QString QpMetaProperty::name() const
{
    return data->metaProperty.name();
}

QString QpMetaProperty::typeName() const
{
    return data->typeName;
}
