#include "metaobject.h"

#include "databaseschema.h"
#include "metaproperty.h"
#include "qpersistence.h"

#include <QDebug>
#include <QHash>
#include <QMetaClassInfo>
#include <QString>

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

    static QHash<QString, QpMetaObject> metaObjects;
};

QHash<QString, QpMetaObject> QpMetaObjectPrivate::metaObjects;

QpMetaObject QpMetaObject::registerMetaObject(const QMetaObject &metaObject)
{
    QString className(metaObject.className());
    auto it = QpMetaObjectPrivate::metaObjects.find(className);

    if (it != QpMetaObjectPrivate::metaObjects.end()) {
        return it.value();
    }

    QpMetaObject result = QpMetaObject(metaObject);
    QpMetaObjectPrivate::metaObjects.insert(className, result);
    return result;
}

QpMetaObject QpMetaObject::forClassName(const QString &className)
{
    auto it = QpMetaObjectPrivate::metaObjects.find(className);
    Q_ASSERT(it != QpMetaObjectPrivate::metaObjects.end());
    return it.value();
}

QList<QpMetaObject> QpMetaObject::registeredMetaObjects()
{
    return QpMetaObjectPrivate::metaObjects.values();
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
    return QString(data->metaObject.className()).toLower();
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


