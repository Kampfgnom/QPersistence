#include "metaproperty.h"

#include "metaobject.h"
#include "qpersistence.h"

#include <QMetaClassInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

static const QRegularExpression TOMANYRELATIONREGEXP("QList\\<(QSharedPointer|QWeakPointer)\\<(\\w+)\\> \\>");
static const QRegularExpression MAPPINGRELATIONREGEXP("QMap\\<(.+),(.+)\\>");
static const QRegularExpression SETTYPEREGEXP("QSet<(.+)\\>");

class QpMetaPropertyPrivate : public QSharedData
{
public:
    QpMetaPropertyPrivate() :
        QSharedData()
    {}

    QpMetaObject metaObject;
    mutable QHash<QString, QString> attributes;

    QpMetaProperty *q;
};

QpMetaProperty::QpMetaProperty(const QString &propertyName, const QpMetaObject &metaObject) :
    QMetaProperty(metaObject.property(metaObject.indexOfProperty(propertyName.toLatin1()))),
    d(new QpMetaPropertyPrivate)
{
    d->q = this;
    d->metaObject = metaObject;
}

QpMetaProperty::QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject) :
    QMetaProperty(property),
    d(new QpMetaPropertyPrivate)
{
    d->q = this;
    d->metaObject = metaObject;
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
    if(!isRelationProperty())
        return QString(name());

    if(cardinality() == ManyToManyCardinality) {
        return QString(metaObject().tableName()).prepend("_Qp_FK_");;
    }
    else if(isToManyRelationProperty()
            || (isToOneRelationProperty()
                && !hasTableForeignKey())) {
        QpMetaProperty reverse(reverseRelation());
        return QString(reverse.name()).prepend("_Qp_FK_");
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
    return QString(typeName()).startsWith("QSharedPointer<")
            || QString(typeName()).startsWith("QWeakPointer<");
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
        return false;

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

    return match.captured(2);
}

QpMetaObject QpMetaProperty::reverseMetaObject() const
{
    return Qp::Private::metaObject(reverseClassName());
}

QString QpMetaProperty::reverseRelationName() const
{
    if(d->attributes.isEmpty()) {
        QString classInfoName = QString(QPERSISTENCE_PROPERTYMETADATA).append(":").append(name());
        QString classInfoRawValue = d->metaObject.classInformation(classInfoName.toLatin1(), QString());

        // First parse the attributes
        QRegularExpression reg("(\\w+)=(\\w+)");
        QStringList attributesList = classInfoRawValue.split(';');
        foreach(const QString attribute, attributesList) {
            QRegularExpressionMatch match = reg.match(attribute);
            if(!match.hasMatch())
                continue;

            d->attributes.insert( match.captured(1), match.captured(2) );
        }
    }

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
    QString s1, s2;

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
        s1 = QString(name());
        s2 = QString(reverseRelation().name());
        if(table > reverseTable) {
            qSwap(table, reverseTable);
            qSwap(s1, s2);
        }
        return QString("_Qp_REL_%1_%2__%3_%4")
                .arg(table)
                .arg(s1)
                .arg(reverseTable)
                .arg(s2);

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


