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


/******************************************************************************
 * QpMetaObjectData
 */
class QpMetaObjectData : public QSharedData
{
public:
    QpMetaObjectData() :
        QSharedData(),
        valid(false)
    {
    }

    bool valid;
    QMetaObject metaObject;
    mutable QList<QpMetaProperty> metaProperties;
    mutable QList<QpMetaProperty> simpleProperties;
    mutable QList<QpMetaProperty> relationProperties;
    mutable QList<QpMetaProperty> calculatedProperties;
    mutable QHash<QString, QpMetaProperty> metaPropertiesByName;

    static QHash<QString, QpMetaObject> metaObjectForName;

    static QSharedDataPointer<QpMetaObjectData> shared_null();
};

QSharedDataPointer<QpMetaObjectData> QpMetaObjectData::shared_null() {
    static QSharedDataPointer<QpMetaObjectData>& shared_null = *new QSharedDataPointer<QpMetaObjectData>(new QpMetaObjectData);
    return shared_null;
}

typedef QHash<QString, QpMetaObject> HashStringToMetaObject;
QP_DEFINE_STATIC_LOCAL(HashStringToMetaObject, MetaObjectsForName)
QP_DEFINE_STATIC_LOCAL(QList<QpMetaObject>, MetaObjects)


/******************************************************************************
 * QpMetaObject
 */
QpMetaObject QpMetaObject::registerMetaObject(const QMetaObject &metaObject)
{
    QString className(metaObject.className());
    auto it = MetaObjectsForName()->find(className);

    if (it != MetaObjectsForName()->end()) {
        return it.value();
    }

    QpMetaObject result = QpMetaObject(metaObject);
    MetaObjects()->append(result);

    const QMetaObject *objectInClassHierarchy = &metaObject;
    do {
        QpMetaObject qpmo = QpMetaObject(*objectInClassHierarchy);
        MetaObjectsForName()->insert(qpmo.classNameWithoutNamespace(), result);
        MetaObjectsForName()->insert(qpmo.className(), result);

        objectInClassHierarchy = objectInClassHierarchy->superClass();
    } while (objectInClassHierarchy->className() != QObject::staticMetaObject.className());

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

QpMetaObject QpMetaObject::forMetaObject(const QMetaObject &metaObject)
{
    return QpMetaObject::forClassName(metaObject.className());
}

QList<QpMetaObject> QpMetaObject::registeredMetaObjects()
{
    return *MetaObjects();
}

QpMetaObject::QpMetaObject() :
    data(QpMetaObjectData::shared_null())
{
}

QpMetaObject::QpMetaObject(const QMetaObject &metaObject) :
    data(new QpMetaObjectData)
{
    data->valid = true;
    data->metaObject = metaObject;
}

QMetaMethod QpMetaObject::method(QString signature, const QpMetaProperty &property) const
{
    QString propertyName = property.name();
    propertyName[0] = propertyName.at(0).toTitleCase();
    QString reverseClassName = property.reverseClassName();
    QMetaMethod method = findMethod(signature.arg(propertyName).arg(reverseClassName));


    if (method.isValid())
        return method;

    QString reverseClassNameWithoutNamespaces = removeNamespaces(reverseClassName);
    if (reverseClassNameWithoutNamespaces == reverseClassName)
        return QMetaMethod();


    method = findMethod(signature.arg(propertyName).arg(reverseClassNameWithoutNamespaces));
    if (method.isValid())
        return method;

    Q_ASSERT_X(false, Q_FUNC_INFO,
               QString("No such method '%1::%2'")
               .arg(data->metaObject.className())

               .arg(signature.arg(propertyName).arg(reverseClassName)).toLatin1());
    return QMetaMethod();
}

QMetaMethod QpMetaObject::findMethod(QString signature) const
{
    QByteArray normalized = QMetaObject::normalizedSignature(signature.toUtf8());
    int index = data->metaObject.indexOfMethod(normalized);

    if (index > 0)
        return data->metaObject.method(index);

    // Remove a possible trailing 's' to match 'many' relations
    signature.remove(signature.indexOf('(') - 1, 1);
    normalized = QMetaObject::normalizedSignature(signature.toUtf8());
    index = data->metaObject.indexOfMethod(normalized);

    if (index > 0)
        return data->metaObject.method(index);

    // Remove a possible trailing 's' to match 'many' relations
    signature.remove(signature.indexOf('(') - 1, 1);
    normalized = QMetaObject::normalizedSignature(signature.toUtf8());
    index = data->metaObject.indexOfMethod(normalized);

    if (index > 0)
        return data->metaObject.method(index);

    // Remove all 's' to even better match 'many' relations
    signature.remove('s');
    normalized = QMetaObject::normalizedSignature(signature.toUtf8());
    index = data->metaObject.indexOfMethod(normalized);

    if (index > 0)
        return data->metaObject.method(index);

    return QMetaMethod();
}

void QpMetaObject::initProperties() const
{
    int count = data->metaObject.propertyCount();
    for (int i = 1; i < count; ++i) {
        QMetaProperty p = data->metaObject.property(i);

        QpMetaProperty mp = QpMetaProperty(p, *this);
        if(mp.isCalculated()) {
            data->calculatedProperties.append(mp);
        }

        if (!p.isStored())
            continue;

        data->metaPropertiesByName.insert(p.name(), mp);
        data->metaProperties.append(mp);
        if (mp.isRelationProperty()) {
            data->relationProperties.append(mp);
        }
        else {
            data->simpleProperties.append(mp);
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

QList<QpMetaProperty> QpMetaObject::calculatedProperties() const
{
    if (data->metaProperties.isEmpty()) {
        initProperties();
    }
    return data->calculatedProperties;
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
    return method("add%1(QSharedPointer<%2>)", property);
}

QMetaMethod QpMetaObject::removeObjectMethod(const QpMetaProperty &property)
{
    return method("remove%1(QSharedPointer<%2>)", property);
}

QString QpMetaObject::removeNamespaces(const QString &classNameWithNamespaces)
{
    QString className = classNameWithNamespaces;
    int namespaceIndex = className.lastIndexOf("::");
    if (namespaceIndex > 0)
        className = className.mid(namespaceIndex + 2);
    return className;
}


bool operator==(const QpMetaObject &a1, const QpMetaObject &a2)
{
    return a1.className() == a2.className();
}
