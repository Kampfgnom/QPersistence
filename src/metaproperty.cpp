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
        QSharedData(),
        cardinality(QpMetaProperty::UnknownCardinality),
        q(nullptr)
    {}

    QString typeName;
    QMetaProperty metaProperty;
    QpMetaObject metaObject;
    mutable QHash<QString, QString> attributes;
    QpMetaProperty::Cardinality cardinality;
    mutable QString columnName;

    QString generateColumnName();

    QpMetaProperty *q;
};

QString QpMetaPropertyPrivate::generateColumnName()
{
    if(!q->isRelationProperty())
        return QString(q->name());

    if(q->cardinality() == QpMetaProperty::ManyToManyCardinality) {
        return QString(q->metaObject().tableName()).prepend("_Qp_FK_");;
    }
    else if(q->isToManyRelationProperty()
            || (q->isToOneRelationProperty()
                && !q->hasTableForeignKey())) {
        QpMetaProperty reverse(q->reverseRelation());
        return QString(reverse.name()).prepend("_Qp_FK_");
    }
    else if(q->isToOneRelationProperty()) {
        return QString(q->name()).prepend("_Qp_FK_");
    }

    return QString(q->name());
}


QpMetaProperty::QpMetaProperty() :
    d(new QpMetaPropertyPrivate)
{
    d->q = this;
}

QpMetaProperty::QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject) :
    d(new QpMetaPropertyPrivate)
{
    d->q = this;
    d->metaProperty = property;
    d->metaObject = metaObject;
    d->typeName = property.typeName();

    Q_ASSERT(isValid());
}

//QpMetaProperty::QpMetaProperty(const QString &propertyName, const QpMetaObject &metaObject) :
//    QMetaProperty(metaObject.property(metaObject.indexOfProperty(propertyName.toLatin1()))),
//    d(new QpMetaPropertyPrivate)
//{
//    d->q = this;
//    d->metaObject = metaObject;
//}

//QpMetaProperty::QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject) :
//    QMetaProperty(property),
//    d(new QpMetaPropertyPrivate)
//{
//    d->q = this;
//    d->metaObject = metaObject;
//}

QpMetaProperty::~QpMetaProperty()
{
}

QpMetaProperty::QpMetaProperty(const QpMetaProperty &other) :
    d(other.d)
{
    d->q = this;
}

QpMetaProperty &QpMetaProperty::operator =(const QpMetaProperty &other)
{
    if(&other != this)
        d.operator =(other.d);

    d->q = this;
    return *this;
}

QpMetaObject QpMetaProperty::metaObject() const
{
    return d->metaObject;
}

QMetaProperty QpMetaProperty::metaProperty() const
{
    return d->metaProperty;
}

QString QpMetaProperty::columnName() const
{
    if(d->columnName.isEmpty())
        d->columnName = d->generateColumnName();

    return d->columnName;
}

bool QpMetaProperty::isStored() const
{
    return d->metaProperty.isStored();
}

bool QpMetaProperty::isValid() const
{
    return d->metaProperty.isValid();
}

QVariant::Type QpMetaProperty::type() const
{
    return d->metaProperty.type();
}

bool QpMetaProperty::isRelationProperty() const
{
    return isToOneRelationProperty() || isToManyRelationProperty();
}

bool QpMetaProperty::isToOneRelationProperty() const
{
    QString type(typeName());
    return type.startsWith("QSharedPointer<")
            || type.startsWith("QWeakPointer<");
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
    return true;
}

QpMetaProperty::Cardinality QpMetaProperty::cardinality() const
{
    if(d->cardinality != UnknownCardinality)
        return d->cardinality;

    QString reverseName = reverseRelationName();
    if(reverseName.isEmpty()) {
        if(isToOneRelationProperty())
            d->cardinality = ToOneCardinality;
        else if(isToManyRelationProperty())
            d->cardinality = ToManyCardinality;
    }
    else {
        QpMetaProperty r = reverseRelation();
        if(isToOneRelationProperty()) {
            if(QString(r.typeName()).isEmpty()) {
                d->cardinality = ToOneCardinality;
            }
            else if(r.isToOneRelationProperty()) {
                d->cardinality = OneToOneCardinality;
            }
            else if(r.isToManyRelationProperty()) {
                d->cardinality = ManyToOneCardinality;
            }
        }
        else if(isToManyRelationProperty()) {
            if(QString(r.typeName()).isEmpty()) {
                d->cardinality = ToManyCardinality;
            }
            else if(r.isToManyRelationProperty()) {
                d->cardinality = ManyToManyCardinality;
            }
            else if(r.isToOneRelationProperty()) {
                d->cardinality = OneToManyCardinality;
            }
        }
    }

    Q_ASSERT_X(d->cardinality != UnknownCardinality, Q_FUNC_INFO,
               QString("The relation %1 has no cardinality. This is an internal error and should never happen.")
               .arg(name())
               .toLatin1());

    return d->cardinality;
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
    return QpMetaObject::forClassName(reverseClassName());
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
    if (!d->metaProperty.isWritable())
        return false;

    QVariant::Type t = d->metaProperty.type();
    if (value.canConvert(t)) {
        QVariant v(value);
        v.convert(t);
        return d->metaProperty.write( obj, v );
    }

    return d->metaProperty.write( obj, value );
}

QString QpMetaProperty::name() const
{
    return d->metaProperty.name();
}

QString QpMetaProperty::typeName() const
{
    return d->typeName;
}
