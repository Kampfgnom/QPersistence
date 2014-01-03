#include "metaobject.h"

#include "metaproperty.h"
#include "qpersistence.h"
#include "databaseschema.h"

#include <QString>
#include <QDebug>
#include <QMetaClassInfo>
#include <QHash>

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

    void initProperties() const;

    QpMetaObject *q;

    static QHash<QString, QpMetaObject> metaObjects;

};
QHash<QString, QpMetaObject> QpMetaObjectPrivate::metaObjects;

void QpMetaObjectPrivate::initProperties() const
{
    int count = metaObject.propertyCount();
    for(int i = 1; i < count; ++i) {
        QMetaProperty p = metaObject.property(i);
        if(!p.isStored())
            continue;

        QpMetaProperty mp = QpMetaProperty(p, *q);
        metaPropertiesByName.insert(p.name(), mp);
        metaProperties.append(mp);

        if(!mp.isRelationProperty()) {
            simpleProperties.append(mp);
        }
        else {
            relationProperties.append(mp);
        }
    }
}


QpMetaObject QpMetaObject::registerMetaObject(const QMetaObject &metaObject)
{
    QString className(metaObject.className());
    auto it = QpMetaObjectPrivate::metaObjects.find(className);

    if(it != QpMetaObjectPrivate::metaObjects.end()) {
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
    d(new QpMetaObjectPrivate)
{
    d->q = this;
}

QpMetaObject::QpMetaObject(const QMetaObject &metaObject) :
    d(new QpMetaObjectPrivate)
{
    d->q = this;
    d->valid = true;
    d->metaObject = metaObject;
}

QpMetaObject::~QpMetaObject()
{
}

QpMetaObject::QpMetaObject(const QpMetaObject &other) :
    d(other.d)
{
    d->q = this;
}

QpMetaObject &QpMetaObject::operator =(const QpMetaObject &other)
{
    if(this != &other) {
        d.operator =(other.d);
        d->q = this;
    }

    return *this;
}

QMetaObject QpMetaObject::metaObject() const
{
    return d->metaObject;
}

QString QpMetaObject::className() const
{
    return d->metaObject.className();
}

bool QpMetaObject::isValid() const
{
    return d->valid;
}

QpMetaProperty QpMetaObject::metaProperty(const QString &name) const
{
    if(d->metaProperties.isEmpty()) {
        d->initProperties();
    }

    auto it = d->metaPropertiesByName.find(name);

    Q_ASSERT_X(it != d->metaPropertiesByName.end(),
               Q_FUNC_INFO,
               qPrintable(QString("The '%1' class has no property '%2'.")
                          .arg(d->metaObject.className())
                          .arg(name)));

    return it.value();
}

QString QpMetaObject::tableName() const
{
    return QString(d->metaObject.className()).toLower();
}

QList<QpMetaProperty> QpMetaObject::metaProperties() const
{
    if(d->metaProperties.isEmpty()) {
        d->initProperties();
    }
    return d->metaProperties;
}

QList<QpMetaProperty> QpMetaObject::simpleProperties() const
{
    if(d->metaProperties.isEmpty()) {
        d->initProperties();
    }
    return d->simpleProperties;
}

QList<QpMetaProperty> QpMetaObject::relationProperties() const
{
    if(d->metaProperties.isEmpty()) {
        d->initProperties();
    }
    return d->relationProperties;
}

QString QpMetaObject::sqlFilter() const
{
    return classInformation(QPERSISTENCE_SQLFILTER, QString());
}

QString QpMetaObject::classInformation(const QString &name, const QString &defaultValue) const
{
    int index = d->metaObject.indexOfClassInfo(name.toLatin1());

    if (index < 0)
        return defaultValue;

    return QLatin1String(d->metaObject.classInfo(index).value());
}

QString QpMetaObject::classInformation(const QString &informationName, bool assertNotEmpty) const
{
    QString value = classInformation(informationName, QString());

    if(assertNotEmpty) {
        Q_ASSERT_X(!value.isEmpty(),
                   Q_FUNC_INFO,
                   qPrintable(QString("The %1 class does not define a %2.")
                              .arg(d->metaObject.className())
                              .arg(informationName)));
    }

    return value;
}


