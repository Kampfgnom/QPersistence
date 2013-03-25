#include "metaproperty.h"

#include "metaobject.h"
#include "qpersistence.h"

#include <QMetaClassInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

static const QRegularExpression TOMANYRELATIONREGEXP("QList\\<(\\w+)\\*\\>");
static const QRegularExpression MAPPINGRELATIONREGEXP("QMap\\<(.+),(.+)\\>");
static const QRegularExpression SETTYPEREGEXP("QSet<(.+)\\>");

class QPersistenceMetaPropertyPrivate : public QSharedData
{
public:
    QPersistenceMetaPropertyPrivate() :
        QSharedData()
    {}

    QPersistenceMetaObject metaObject;
    QHash<QString, QString> attributes;

    QPersistenceMetaProperty *q;

    void parseClassInfo();
};

void QPersistenceMetaPropertyPrivate::parseClassInfo()
{
    QString classInfoName = QString(QPERSISTENCE_PROPERTYMETADATA).append(":").append(q->name());
    QString classInfoRawValue = metaObject.classInformation(classInfoName.toLatin1(), QString());

    // First parse the attributes
    QRegularExpression reg("(\\w+)=(\\w+)");
    QStringList attributesList = classInfoRawValue.split(';');
    foreach(const QString attribute, attributesList) {
        QRegularExpressionMatch match = reg.match(attribute);
        if(!match.hasMatch())
            continue;

        attributes.insert( match.captured(1), match.captured(2) );
    }
}

QPersistenceMetaProperty::QPersistenceMetaProperty(const QString &propertyName, const QPersistenceMetaObject &metaObject) :
    QMetaProperty(metaObject.property(metaObject.indexOfProperty(propertyName.toLatin1()))),
    d(new QPersistenceMetaPropertyPrivate)
{
    d->q = this;
    d->metaObject = metaObject;
    d->parseClassInfo();
}

QPersistenceMetaProperty::QPersistenceMetaProperty(const QMetaProperty &property, const QPersistenceMetaObject &metaObject) :
    QMetaProperty(property),
    d(new QPersistenceMetaPropertyPrivate)
{
    d->q = this;
    d->metaObject = metaObject;
    d->parseClassInfo();
}

QPersistenceMetaProperty::~QPersistenceMetaProperty()
{
}

QPersistenceMetaProperty::QPersistenceMetaProperty(const QPersistenceMetaProperty &other) :
    QMetaProperty(other),
    d(other.d)
{
}

QPersistenceMetaProperty &QPersistenceMetaProperty::operator =(const QPersistenceMetaProperty &other)
{
    QMetaProperty::operator=(other);

    if(&other != this)
        d.operator =(other.d);

    return *this;
}

QPersistenceMetaObject QPersistenceMetaProperty::metaObject() const
{
    return d->metaObject;
}

bool QPersistenceMetaProperty::isAutoIncremented() const
{
    return d->attributes.value(QPERSISTENCE_PROPERTYMETADATA_AUTOINCREMENTED) == QLatin1String(QPERSISTENCE_TRUE);
}

QString QPersistenceMetaProperty::columnName() const
{
    if(d->attributes.contains(QPERSISTENCE_PROPERTYMETADATA_SQL_COLUMNNAME))
        return d->attributes.value(QPERSISTENCE_PROPERTYMETADATA_SQL_COLUMNNAME);

    if(isToManyRelationProperty()) {
        QString result = QString(name());

        QPersistenceMetaProperty reverse(reverseRelation());
        if(reverse.isValid()) {
            result = QString(reverse.name());
        }

        result.append("_fk_").append(reverseMetaObject().primaryKeyPropertyName());
        return result;
    }
    else if(isToOneRelationProperty()) {
        return QString(name())
                .append("_fk_")
                .append(reverseMetaObject().primaryKeyPropertyName());
    }

    return QString(name());
}

bool QPersistenceMetaProperty::isReadOnly() const
{
    return d->attributes.value(QPERSISTENCE_PROPERTYMETADATA_READONLY) == QLatin1String(QPERSISTENCE_TRUE);
}

bool QPersistenceMetaProperty::isPrimaryKey() const
{
    return d->metaObject.primaryKeyPropertyName() == QLatin1String(name());
}

bool QPersistenceMetaProperty::isRelationProperty() const
{
    return isToOneRelationProperty() || isToManyRelationProperty();
}

bool QPersistenceMetaProperty::isToOneRelationProperty() const
{
    return QString(typeName()).endsWith('*');
}

bool QPersistenceMetaProperty::isToManyRelationProperty() const
{
    return TOMANYRELATIONREGEXP.match(typeName()).hasMatch();
}

QPersistenceMetaProperty::Cardinality QPersistenceMetaProperty::cardinality() const
{
    QString reverseName = reverseRelationName();
    if(reverseName.isEmpty()) {
        if(isToOneRelationProperty())
            return ToOneCardinality;
        else if(isToManyRelationProperty())
            return ToManyCardinality;
    }
    else {
        QPersistenceMetaProperty reverse = reverseMetaObject().metaProperty(reverseName);

        if(isToOneRelationProperty()) {
            if(!reverse.isValid() ||
                    QString(reverse.typeName()).isEmpty()) {
                return ToOneCardinality;
            }
            else if(reverse.isToOneRelationProperty()) {
                return OneToOneCardinality;
            }
            else if(reverse.isToManyRelationProperty()) {
                return ManyToOneCardinality;
            }
        }
        else if(isToManyRelationProperty()) {
            if(!reverse.isValid() ||
                    QString(reverse.typeName()).isEmpty()) {
                return ToManyCardinality;
            }
            else if(reverse.isToManyRelationProperty()) {
                return ManyToManyCardinality;
            }
            else if(reverse.isToOneRelationProperty()) {
                return OneToManyCardinality;
            }
        }
    }

    Q_ASSERT_X(false, Q_FUNC_INFO,
               QString("The relation %1 has no cardinality. This is an internal error and should never happen.")
               .arg(name())
               .toLatin1());
    return NoCardinality;
}

QString QPersistenceMetaProperty::reverseClassName() const
{
    QString name(typeName());
    if(isToOneRelationProperty()) {
        return name.left(name.length() - 1);
    }

    QRegularExpressionMatch match = TOMANYRELATIONREGEXP.match(name);
    if(!match.hasMatch())
        return QString();

    return match.captured(1);
}

QPersistenceMetaObject QPersistenceMetaProperty::reverseMetaObject() const
{
    return QPersistence::metaObject(reverseClassName());
}

QString QPersistenceMetaProperty::reverseRelationName() const
{
    return d->attributes.value(QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION);
}

QPersistenceMetaProperty QPersistenceMetaProperty::reverseRelation() const
{
    return reverseMetaObject().metaProperty(reverseRelationName());
}

QString QPersistenceMetaProperty::tableName() const
{
    if(!isRelationProperty())
        return metaObject().tableName();

    switch(cardinality()) {
    case QPersistenceMetaProperty::ToOneCardinality:
    case QPersistenceMetaProperty::ManyToOneCardinality:
        // My table gets a foreign key column
        return metaObject().tableName();

    case QPersistenceMetaProperty::ToManyCardinality:
    case QPersistenceMetaProperty::OneToManyCardinality:
        // The related table gets a foreign key column
        return reverseMetaObject().tableName();

    case QPersistenceMetaProperty::OneToOneCardinality:
        Q_ASSERT_X(false, Q_FUNC_INFO, "OneToOneCardinality relations are not supported yet.");

    case QPersistenceMetaProperty::ManyToManyCardinality:
        Q_ASSERT_X(false, Q_FUNC_INFO, "ManyToManyCardinality relations are not supported yet.");

    case QPersistenceMetaProperty::NoCardinality:
    default:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    return QString();
}

QVariant::Type QPersistenceMetaProperty::foreignKeyType() const
{
    if(!isRelationProperty())
        return QVariant::Invalid;

    switch(cardinality()) {
    case QPersistenceMetaProperty::ToOneCardinality:
    case QPersistenceMetaProperty::ManyToOneCardinality:
        // My table gets a foreign key column: I.e. the foreign key type is the type of the related primary key
        return reverseMetaObject().primaryKeyProperty().type();

    case QPersistenceMetaProperty::ToManyCardinality:
    case QPersistenceMetaProperty::OneToManyCardinality:
        // The related table gets a foreign key column: I.e. the foreign key type is the type of my primary key
        return metaObject().primaryKeyProperty().type();

    case QPersistenceMetaProperty::OneToOneCardinality:
        Q_ASSERT_X(false, Q_FUNC_INFO, "OneToOneCardinality relations are not supported yet.");

    case QPersistenceMetaProperty::ManyToManyCardinality:
        Q_ASSERT_X(false, Q_FUNC_INFO, "ManyToManyCardinality relations are not supported yet.");

    case QPersistenceMetaProperty::NoCardinality:
    default:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    return QVariant::Invalid;
}

bool QPersistenceMetaProperty::isMappingProperty() const
{
    return QString(typeName()).startsWith("QMap");
}

QString QPersistenceMetaProperty::mappingFromTypeName() const
{
    QRegularExpressionMatch match = MAPPINGRELATIONREGEXP.match(typeName());
    if(!match.hasMatch())
        return QString();

    return match.captured(1);
}

QString QPersistenceMetaProperty::mappingToTypeName() const
{
    QRegularExpressionMatch match = MAPPINGRELATIONREGEXP.match(typeName());
    if(!match.hasMatch())
        return QString();

    return match.captured(2);
}

bool QPersistenceMetaProperty::isSetProperty() const
{
    return QString(typeName()).startsWith("QSet");
}

QString QPersistenceMetaProperty::setType() const
{
    QRegularExpressionMatch match = SETTYPEREGEXP.match(typeName());
    if(!match.hasMatch())
        return QString();

    return match.captured(2);
}

bool QPersistenceMetaProperty::write(QObject *obj, const QVariant &value) const
{
    if (!isWritable())
        return false;

    QVariant::Type t = type();
    if (value.canConvert(t)) {
        QVariant v(value);
        v.convert(t);
        return QMetaProperty::write( obj, v );
    }

    return QMetaProperty::write( obj, value );
}


