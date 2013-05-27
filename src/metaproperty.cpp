#include "metaproperty.h"

#include "metaobject.h"
#include "qpersistence.h"

#include <QMetaClassInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

static const QRegularExpression TOMANYRELATIONREGEXP("QList\\<QSharedPointer\\<(\\w+)\\> \\>");
static const QRegularExpression MAPPINGRELATIONREGEXP("QMap\\<(.+),(.+)\\>");
static const QRegularExpression SETTYPEREGEXP("QSet<(.+)\\>");

class QpMetaPropertyPrivate : public QSharedData
{
public:
    QpMetaPropertyPrivate() :
        QSharedData()
    {}

    QpMetaObject metaObject;
    QHash<QString, QString> attributes;

    QpMetaProperty *q;

    void parseClassInfo();
};

void QpMetaPropertyPrivate::parseClassInfo()
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

QpMetaProperty::QpMetaProperty(const QString &propertyName, const QpMetaObject &metaObject) :
    QMetaProperty(metaObject.property(metaObject.indexOfProperty(propertyName.toLatin1()))),
    d(new QpMetaPropertyPrivate)
{
    d->q = this;
    d->metaObject = metaObject;
    d->parseClassInfo();
}

QpMetaProperty::QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject) :
    QMetaProperty(property),
    d(new QpMetaPropertyPrivate)
{
    d->q = this;
    d->metaObject = metaObject;
    d->parseClassInfo();
}

QpMetaProperty::~QpMetaProperty()
{
}

QpMetaProperty::QpMetaProperty(const QpMetaProperty &other) :
    QMetaProperty(other),
    d(other.d)
{
}

QpMetaProperty &QpMetaProperty::operator =(const QpMetaProperty &other)
{
    QMetaProperty::operator=(other);

    if(&other != this)
        d.operator =(other.d);

    return *this;
}

QpMetaObject QpMetaProperty::metaObject() const
{
    return d->metaObject;
}

QString QpMetaProperty::columnName() const
{
    if(isToManyRelationProperty()
            || (isToOneRelationProperty()
                && !hasTableForeignKey())) {
        QString result = QString(name());

        QpMetaProperty reverse(reverseRelation());
        if(reverse.isValid()) {
            result = QString(reverse.name());
        }

        result.prepend("_Qp_FK_");
        return result;
    }
    else if(isToOneRelationProperty()) {
        return QString(name()).prepend("_Qp_FK_");
    }

    return QString(name());
}

bool QpMetaProperty::isRelationProperty() const
{
    return isToOneRelationProperty() || isToManyRelationProperty();
}

bool QpMetaProperty::isToOneRelationProperty() const
{
    return QString(typeName()).startsWith("QSharedPointer<");
}

bool QpMetaProperty::isToManyRelationProperty() const
{
    return TOMANYRELATIONREGEXP.match(typeName()).hasMatch();
}

bool QpMetaProperty::hasTableForeignKey() const
{
    switch(cardinality()) {
    case QpMetaProperty::ToOneCardinality:
    case QpMetaProperty::ManyToOneCardinality:
        return true;

    case QpMetaProperty::ToManyCardinality:
    case QpMetaProperty::OneToManyCardinality:
        return false;

    case QpMetaProperty::OneToOneCardinality:
        return  metaObject().tableName() < reverseMetaObject().tableName();

    case QpMetaProperty::ManyToManyCardinality:
        Q_ASSERT_X(false, Q_FUNC_INFO, "ManyToManyCardinality relations are not supported yet.");

    case QpMetaProperty::NoCardinality:
    default:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    Q_ASSERT(false);
}

QpMetaProperty::Cardinality QpMetaProperty::cardinality() const
{
    QString reverseName = reverseRelationName();
    if(reverseName.isEmpty()) {
        if(isToOneRelationProperty())
            return ToOneCardinality;
        else if(isToManyRelationProperty())
            return ToManyCardinality;
    }
    else {
        QpMetaProperty reverse = reverseMetaObject().metaProperty(reverseName);

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

QString QpMetaProperty::reverseClassName() const
{
    QString name(typeName());
    if(isToOneRelationProperty()) {
        int l = name.length();
        return name.left(l - 1).right(l - 16);
    }

    QRegularExpressionMatch match = TOMANYRELATIONREGEXP.match(name);
    if(!match.hasMatch())
        return QString();

    return match.captured(1);
}

QpMetaObject QpMetaProperty::reverseMetaObject() const
{
    return Qp::Private::metaObject(reverseClassName());
}

QString QpMetaProperty::reverseRelationName() const
{
    return d->attributes.value(QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION);
}

QpMetaProperty QpMetaProperty::reverseRelation() const
{
    return reverseMetaObject().metaProperty(reverseRelationName());
}

QString QpMetaProperty::tableName() const
{
    if(!isRelationProperty())
        return metaObject().tableName();

    QString table = metaObject().tableName();
    QString reverseTable = reverseMetaObject().tableName();

    switch(cardinality()) {
    case QpMetaProperty::ToOneCardinality:
    case QpMetaProperty::ManyToOneCardinality:
        // My table gets a foreign key column
        return table;

    case QpMetaProperty::ToManyCardinality:
    case QpMetaProperty::OneToManyCardinality:
        // The related table gets a foreign key column
        return reverseTable;

    case QpMetaProperty::OneToOneCardinality:
        return table < reverseTable ? table : reverseTable;

    case QpMetaProperty::ManyToManyCardinality:
        Q_ASSERT_X(false, Q_FUNC_INFO, "ManyToManyCardinality relations are not supported yet.");

    case QpMetaProperty::NoCardinality:
    default:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    return QString();
}

bool QpMetaProperty::isMappingProperty() const
{
    return QString(typeName()).startsWith("QMap");
}

QString QpMetaProperty::mappingFromTypeName() const
{
    QRegularExpressionMatch match = MAPPINGRELATIONREGEXP.match(typeName());
    if(!match.hasMatch())
        return QString();

    return match.captured(1);
}

QString QpMetaProperty::mappingToTypeName() const
{
    QRegularExpressionMatch match = MAPPINGRELATIONREGEXP.match(typeName());
    if(!match.hasMatch())
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
    if(!match.hasMatch())
        return QString();

    return match.captured(2);
}

bool QpMetaProperty::write(QObject *obj, const QVariant &value) const
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


