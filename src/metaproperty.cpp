#include "metaproperty.h"

#include "metaobject.h"
#include "qpersistence.h"
#include "relations.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QMetaClassInfo>
#include <QRegularExpression>
#include <QStringList>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

static const char* TOMANYRELATIONREGEXP("QList\\<(QSharedPointer|QWeakPointer)\\<(.+)\\> \\>");
static const char* MAPPINGRELATIONREGEXP("QMap\\<(.+),(.+)\\>");
static const char* SETTYPEREGEXP("QSet<(.+)\\>");


/******************************************************************************
 * QpMetaPropertyData
 */
class QpMetaPropertyData : public QSharedData
{
public:
    QpMetaPropertyData() :
        QSharedData(),
        mutex(new QMutex(QMutex::Recursive)),
        cardinality(QpMetaProperty::UnknownCardinality)
    {
    }

    QMutex *mutex;
    QpMetaProperty::Cardinality cardinality;
    QString typeName;
    QMetaProperty metaProperty;
    QpMetaObject metaObject;
    mutable QHash<QString, QString> attributes;
    mutable QString columnName;

    static QSharedDataPointer<QpMetaPropertyData> shared_null();
};

QSharedDataPointer<QpMetaPropertyData> QpMetaPropertyData::shared_null() {
    static QSharedDataPointer<QpMetaPropertyData>& shared_null = *new QSharedDataPointer<QpMetaPropertyData>(new QpMetaPropertyData);
    return shared_null;
}

/******************************************************************************
 * QpMetaProperty
 */
QString QpMetaProperty::nameFromMaybeQualifiedName(const QString &maybeQualifiedName)
{
    int classNameEndIndex = maybeQualifiedName.lastIndexOf("::");
    QString n = maybeQualifiedName;
    if (classNameEndIndex >= 0)
        n = maybeQualifiedName.mid(classNameEndIndex + 2);
    return n;
}

QpMetaProperty::QpMetaProperty() :
    data(QpMetaPropertyData::shared_null())
{
}

QpMetaProperty::QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject) :
    data(new QpMetaPropertyData)
{
    data->metaObject = metaObject;
    data->typeName = property.typeName();
    data->metaProperty = property;

    Q_ASSERT(isValid());
}

QString QpMetaProperty::generateColumnName() const
{
    if (!isRelationProperty())
        return QString(name());

    if (cardinality() == QpMetaProperty::ManyToManyCardinality) {
        QString relationName;

        if (reverseMetaObject().className() == data->metaObject.className())
            relationName = name();
        else
            relationName = data->metaObject.tableName();

        return relationName.prepend("_Qp_FK_");;
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
    if (result.size() > 56)
        result = result.left(56);

    return result;
}

void QpMetaProperty::parseAttributes() const
{
    QMutexLocker m(data->mutex); Q_UNUSED(m);
    if(!data->attributes.isEmpty())
        return;

    QString classInfoName = QString(QPERSISTENCE_PROPERTYMETADATA).append(":").append(name());
    QString classInfoRawValue = data->metaObject.classInformation(classInfoName.toLatin1(), QString());

    // First parse the attributes
    QRegularExpression reg("([^=]+)=([^=]+)");
    QStringList attributesList = classInfoRawValue.split(';');
    foreach (const QString attribute, attributesList) {
        QRegularExpressionMatch match = reg.match(attribute);
        if (!match.hasMatch())
            continue;

        data->attributes.insert( match.captured(1), match.captured(2) );
    }
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

bool QpMetaProperty::isLazy() const
{
    return hasAnnotation("lazy");
}

QVariant::Type QpMetaProperty::type() const
{
    return data->metaProperty.type();
}

bool QpMetaProperty::isCalculated() const
{
    return attributes().contains("depends");
}

QStringList QpMetaProperty::dependencies() const
{
    return attributes().value("depends").split(',');
}

QMetaMethod QpMetaProperty::recalculateMethod(QSharedPointer<QObject> object) const
{
    const QMetaObject *metaObject = object->metaObject();
    QString methodName = data->metaProperty.name();
    methodName[0] = methodName.at(0).toTitleCase();
    methodName.prepend("recalculate");

    QByteArray signature = QMetaObject::normalizedSignature(methodName.toLatin1());
    int index = metaObject->indexOfMethod(signature);
    Q_ASSERT_X(index >= 0,
               Q_FUNC_INFO,
               QString::fromLatin1("No recalculate method found for property '%1'").arg(data->metaProperty.name()).toLatin1());
    return metaObject->method(index);
}

QHash<QString, QString> QpMetaProperty::attributes() const
{
    parseAttributes();

    return data->attributes;
}

bool QpMetaProperty::isRelationProperty() const
{
    return isToOneRelationProperty() || isToManyRelationProperty();
}

bool QpMetaProperty::isToOneRelationProperty() const
{
    if (data->cardinality == UnknownCardinality) {
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
    if (data->cardinality == UnknownCardinality)
        return QRegularExpression(TOMANYRELATIONREGEXP).match(typeName()).hasMatch();

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
        return metaObject().tableName() < reverseMetaObject().tableName();

    case QpMetaProperty::ManyToManyCardinality:
        return false;

    case UnknownCardinality:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    Q_ASSERT(false);
    return true;
}

bool QpMetaProperty::hasAnnotation(const QString &name) const
{
    return QVariant(attributes().value(name, "false")).toBool();
}

QpMetaProperty::Cardinality QpMetaProperty::cardinality() const
{
    QMutexLocker m(data->mutex); Q_UNUSED(m);
    if (data->cardinality != UnknownCardinality)
        return data->cardinality;

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

    QRegularExpressionMatch match = QRegularExpression(TOMANYRELATIONREGEXP).match(name);
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
    QString reverse = attributes().value(QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION);
    if (!reverse.isEmpty())
        return reverse;

    QList<QpMetaProperty> possibleReverses;
    foreach (QpMetaProperty relation, reverseMetaObject().relationProperties()) {
        if (relation.reverseClassName() == metaObject().className()) {
            possibleReverses << relation;
        }
    }

    Q_ASSERT_X(!possibleReverses.isEmpty(), Q_FUNC_INFO,
               QString("The %1::%2 relation must have a reverse in %3")
               .arg(metaObject().className())
               .arg(name())
               .arg(reverseClassName())
               .toLatin1());
    Q_ASSERT_X(possibleReverses.size() == 1, Q_FUNC_INFO,
               QString("You must explicitly specify the reverse relation for %1::%2 in %3, since it it ambigious.")
               .arg(metaObject().className())
               .arg(name())
               .arg(reverseClassName())
               .toLatin1());
    return possibleReverses.first().name();
}

QpMetaProperty QpMetaProperty::reverseRelation() const
{
    return reverseMetaObject().metaProperty(reverseRelationName());
}

QString QpMetaProperty::internalRelationObjectPropertyName() const
{
    return name().append("_internal");
}

QpRelationBase *QpMetaProperty::internalRelationObject(const QObject *object) const
{
    return object->property(internalRelationObjectPropertyName().toLatin1()).value<QpRelationBase *>();
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
        if (table == reverseTable
            && s1 < s2) {
            qSwap(table, reverseTable);
            qSwap(s1, s2);
        }
        result = QString("_Qp_REL_%1_%2__%3_%4")
                 .arg(table)
                 .arg(s1)
                 .arg(reverseTable)
                 .arg(s2);
        break;

    case UnknownCardinality:
        // This is BAD and should have asserted in cardinality()
        Q_ASSERT(false);
    }

    return shortName(result);
}

void QpMetaProperty::remove(QSharedPointer<QObject> object, QSharedPointer<QObject> related) const
{
    if (isToOneRelationProperty()) {
        data->metaProperty.write(object.data(), Qp::Private::variantCast(QSharedPointer<QObject>(), reverseClassName()));
    }
    else {
        QVariant wrapper = Qp::Private::variantCast(related, reverseClassName());

        const QMetaObject *mo = object->metaObject();
        QByteArray methodName = data->metaObject.removeObjectMethod(*this).methodSignature();
        int index = mo->indexOfMethod(methodName);

        Q_ASSERT_X(index > 0, Q_FUNC_INFO,
                   QString("You have to add a public slot with the signature '%1' to your '%2' class!")
                   .arg(QString::fromLatin1(methodName))
                   .arg(mo->className())
                   .toLatin1());

        QMetaMethod method = mo->method(index);
        bool result = method.invoke(object.data(), Qt::DirectConnection,
                                    QGenericArgument(data->metaProperty.typeName(), wrapper.data()));
        Q_ASSERT(result);
        Q_UNUSED(result);
    }
}

void QpMetaProperty::add(QSharedPointer<QObject> object, QSharedPointer<QObject> related) const
{
    if (isToOneRelationProperty()) {
        data->metaProperty.write(object.data(), Qp::Private::variantCast(related, reverseClassName()));
    }
    else {
        QVariant wrapper = Qp::Private::variantCast(related, reverseClassName());

        const QMetaObject *mo = object->metaObject();
        QByteArray methodName = data->metaObject.addObjectMethod(*this).methodSignature();
        int index = mo->indexOfMethod(methodName);

        Q_ASSERT_X(index > 0, Q_FUNC_INFO,
                   QString("You have to add a public slot with the signature '%1' to your '%2' class!")
                   .arg(QString::fromLatin1(methodName))
                   .arg(mo->className())
                   .toLatin1());

        QMetaMethod method = mo->method(index);
        bool result = method.invoke(object.data(), Qt::DirectConnection,
                                    QGenericArgument(data->metaProperty.typeName(), wrapper.data()));
        Q_ASSERT(result);
        Q_UNUSED(result);
    }
}

QSharedPointer<QObject> QpMetaProperty::readOne(const QObject *object) const
{
    QList<QSharedPointer<QObject> > os = read(object);
    if(os.isEmpty())
        return QSharedPointer<QObject>();

    Q_ASSERT(os.size() == 1);
    return os.first();
}

QSharedPointer<QObject> QpMetaProperty::readOne(QSharedPointer<QObject> object) const
{
    return readOne(object.data());
}

QList<QSharedPointer<QObject> > QpMetaProperty::read(const QObject *object) const
{
    switch (cardinality()) {
    case QpMetaProperty::OneToOneCardinality:
    case QpMetaProperty::ManyToOneCardinality:
        return { Qp::Private::objectCast(data->metaProperty.read(object)) };

    case QpMetaProperty::OneToManyCardinality:
    case QpMetaProperty::ManyToManyCardinality:
        return Qp::Private::objectListCast(data->metaProperty.read(object));

    case QpMetaProperty::UnknownCardinality:
        return {};
    }

    return {};
}

QList<QSharedPointer<QObject> > QpMetaProperty::read(QSharedPointer<QObject> object) const
{
    return read(object.data());
}

bool QpMetaProperty::isRelated(QSharedPointer<QObject> left, QSharedPointer<QObject> right) const
{
    QVariant value = data->metaProperty.read(left.data());

    switch (cardinality()) {
    case QpMetaProperty::UnknownCardinality:
        return false;

    case QpMetaProperty::OneToOneCardinality:
    case QpMetaProperty::ManyToOneCardinality: {
        QSharedPointer<QObject> related = Qp::Private::objectCast(value);
        return related == right;
    }
    case QpMetaProperty::OneToManyCardinality:
    case QpMetaProperty::ManyToManyCardinality: {
        QList<QSharedPointer<QObject > > objects = Qp::Private::objectListCast(value);
        return objects.contains(right);
    }
    }

    return false;
}

bool QpMetaProperty::isMappingProperty() const
{
    return QString(typeName()).startsWith("QMap");
}

QString QpMetaProperty::mappingFromTypeName() const
{
    QRegularExpressionMatch match = QRegularExpression(MAPPINGRELATIONREGEXP).match(typeName());
    if (!match.hasMatch())
        return QString();

    return match.captured(1);
}

QString QpMetaProperty::mappingToTypeName() const
{
    QRegularExpressionMatch match = QRegularExpression(MAPPINGRELATIONREGEXP).match(typeName());
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
    QRegularExpressionMatch match = QRegularExpression(SETTYPEREGEXP).match(typeName());
    if (!match.hasMatch())
        return QString();

    return match.captured(2);
}

bool QpMetaProperty::write(QObject *obj, const QVariant &value) const
{
    if (!data->metaProperty.isWritable())
        return false;

    QVariant::Type t = data->metaProperty.type();
    if (value.canConvert(static_cast<int>(t))) {
        QVariant v(value);
        v.convert(static_cast<int>(t));
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


uint qHash(const QpMetaProperty &t, uint seed)
{
    return qHash(t.metaObject().className(), seed) ^ qHash(t.metaProperty().name(), seed);
}

bool operator==(const QpMetaProperty &a1, const QpMetaProperty &a2)
{
    return a1.name() == a2.name() && a1.metaObject() == a2.metaObject();
}
