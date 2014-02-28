#include "metaobject.h"

#include "databaseschema.h"
#include "metaproperty.h"
#include "qpersistence.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QHash>
#include <QMetaClassInfo>
#include <QSharedData>
#include <QString>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpMetaObjectPrivate : public QSharedData
{
public:
    QpMetaObjectPrivate() :
        QSharedData(),
        valid(false)
    {}

    bool valid;
    QMetaObject metaObject;
    mutable QList<QpMetaProperty> metaProperties;
    mutable QList<QpMetaProperty> simpleProperties;
    mutable QList<QpMetaProperty> relationProperties;
    mutable QHash<QString, QpMetaProperty> metaPropertiesByName;

    static QHash<QString, QpMetaObject> metaObjectForName;
};

typedef QHash<QString, QpMetaObject> HashStringToMetaObject;
QP_DEFINE_STATIC_LOCAL(HashStringToMetaObject, MetaObjectsForName)
QP_DEFINE_STATIC_LOCAL(QList<QpMetaObject>, MetaObjects)

QpMetaObject QpMetaObject::registerMetaObject(const QMetaObject &metaObject)
{
    QString className(metaObject.className());
    auto it = MetaObjectsForName()->find(className);

    if (it != MetaObjectsForName()->end()) {
        return it.value();
    }

    QpMetaObject result = QpMetaObject(metaObject);
    MetaObjectsForName()->insert(result.classNameWithoutNamespace(), result);
    MetaObjectsForName()->insert(className, result);
    MetaObjects()->append(result);

    return result;
}

QpMetaObject QpMetaObject::forObject(const QObject *object)
{
    return forClassName(object->metaObject()->className());
}

QpMetaObject QpMetaObject::forObject(QSharedPointer<QObject> object)
{
    return forObject(object.data());
}

QpMetaObject QpMetaObject::forClassName(const QString &className)
{
    auto it = MetaObjectsForName()->find(className);
    Q_ASSERT_X(it != MetaObjectsForName()->end(),
               Q_FUNC_INFO,
               QString("No such metaobject for class name '%1'!\nHave you forgotten to register the class?")
               .arg(className)
               .toUtf8());
    return it.value();
}

QList<QpMetaObject> QpMetaObject::registeredMetaObjects()
{
    return *MetaObjects();
}

QpMetaObject::QpMetaObject() :
    data(new QpMetaObjectPrivate)
{
}

QpMetaObject::QpMetaObject(const QMetaObject &metaObject) :
    data(new QpMetaObjectPrivate)
{
    data->valid = true;
    data->metaObject = metaObject;
}

void QpMetaObject::initProperties() const
{
    int count = data->metaObject.propertyCount();
    for (int i = 1; i < count; ++i) {
        QMetaProperty p = data->metaObject.property(i);
        if (!p.isStored())
            continue;

        QpMetaProperty mp = QpMetaProperty(p, *this);
        data->metaPropertiesByName.insert(p.name(), mp);
        data->metaProperties.append(mp);

        if (!mp.isRelationProperty()) {
            data->simpleProperties.append(mp);
        }
        else {
            data->relationProperties.append(mp);
        }
    }
}

QpMetaObject::~QpMetaObject()
{
}

QpMetaObject::QpMetaObject(const QpMetaObject &other) :
    data(other.data)
{
}

QpMetaObject &QpMetaObject::operator =(const QpMetaObject &other)
{
    if (this != &other) {
        data.operator =(other.data);
    }

    return *this;
}

QMetaObject QpMetaObject::metaObject() const
{
    return data->metaObject;
}

QString QpMetaObject::className() const
{
    return data->metaObject.className();
}

QString QpMetaObject::classNameWithoutNamespace() const
{
    return removeNamespaces(data->metaObject.className());
}

bool QpMetaObject::isValid() const
{
    return data->valid;
}

QpMetaProperty QpMetaObject::metaProperty(const QString &name) const
{
    if (data->metaProperties.isEmpty()) {
        initProperties();
    }

    auto it = data->metaPropertiesByName.find(name);

    Q_ASSERT_X(it != data->metaPropertiesByName.end(),
               Q_FUNC_INFO,
               qPrintable(QString("The '%1' class has no property '%2'.")
                          .arg(data->metaObject.className())
                          .arg(name)));

    return it.value();
}

QString QpMetaObject::tableName() const
{
    return classNameWithoutNamespace().toLower();
}

QList<QpMetaProperty> QpMetaObject::metaProperties() const
{
    if (data->metaProperties.isEmpty()) {
        initProperties();
    }
    return data->metaProperties;
}

QList<QpMetaProperty> QpMetaObject::simpleProperties() const
{
    if (data->metaProperties.isEmpty()) {
        initProperties();
    }
    return data->simpleProperties;
}

QList<QpMetaProperty> QpMetaObject::relationProperties() const
{
    if (data->metaProperties.isEmpty()) {
        initProperties();
    }
    return data->relationProperties;
}

QString QpMetaObject::sqlFilter() const
{
    return classInformation(QPERSISTENCE_SQLFILTER, QString());
}

QString QpMetaObject::classInformation(const QString &name, const QString &defaultValue) const
{
    int index = data->metaObject.indexOfClassInfo(name.toLatin1());

    if (index < 0)
        return defaultValue;

    return QLatin1String(data->metaObject.classInfo(index).value());
}

QString QpMetaObject::classInformation(const QString &informationName, bool assertNotEmpty) const
{
    QString value = classInformation(informationName, QString());

    if (assertNotEmpty) {
        Q_ASSERT_X(!value.isEmpty(),
                   Q_FUNC_INFO,
                   qPrintable(QString("The %1 class does not define a %2.")
                              .arg(data->metaObject.className())
                              .arg(informationName)));
    }

    return value;
}

QMetaMethod QpMetaObject::addObjectMethod(const QpMetaProperty &property)
{
    QString signature("add%1(QSharedPointer<%2>)");
    QString propertyName = property.name();
    propertyName[0] = propertyName.at(0).toTitleCase();
    signature = signature.arg(propertyName).arg(property.reverseClassName());
    QByteArray normalized = QMetaObject::normalizedSignature(signature.toUtf8());
    int index = data->metaObject.indexOfMethod(normalized);

    Q_ASSERT_X(index > 0, Q_FUNC_INFO,
               QString("No such method '%1::%2'")
               .arg(data->metaObject.className())
               .arg(signature).toLatin1());
    return data->metaObject.method(index);
}

QString QpMetaObject::removeNamespaces(const QString &classNameWithNamespaces)
{
    QString className = classNameWithNamespaces;
    int namespaceIndex = className.lastIndexOf("::");
    if(namespaceIndex > 0)
        className = className.mid(namespaceIndex + 2);
    return className;
}

QMetaMethod QpMetaObject::removeObjectMethod(const QpMetaProperty &property)
{
    QString signature("remove%1(QSharedPointer<%2>)");
    QString propertyName = property.name();
    propertyName[0] = propertyName.at(0).toTitleCase();
    signature = signature.arg(propertyName).arg(property.reverseClassName());
    QByteArray normalized = QMetaObject::normalizedSignature(signature.toUtf8());
    int index = data->metaObject.indexOfMethod(normalized);

    Q_ASSERT_X(index > 0, Q_FUNC_INFO,
               QString("No such method '%1::%2'")
               .arg(data->metaObject.className())
               .arg(signature).toLatin1());
    return data->metaObject.method(index);
}
