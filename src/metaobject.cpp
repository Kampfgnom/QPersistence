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
        QSharedData()
    {}
};

QpMetaObject::QpMetaObject() :
    QMetaObject(),
    d(new QpMetaObjectPrivate)
{
}

QpMetaObject::QpMetaObject(const QMetaObject &metaObject) :
    QMetaObject(metaObject),
    d(new QpMetaObjectPrivate)
{
}

QpMetaObject::~QpMetaObject()
{
}

QpMetaObject::QpMetaObject(const QpMetaObject &other) :
    QMetaObject(other),
    d(other.d)
{
}

QpMetaObject &QpMetaObject::operator =(const QpMetaObject &other)
{
    QMetaObject::operator=(other);

    if(this != &other) {
        d = other.d;
    }

    return *this;
}

QpMetaProperty QpMetaObject::metaProperty(const QString &name) const
{
    int index = indexOfProperty(name.toLatin1());

    Q_ASSERT_X(index >= 0,
               Q_FUNC_INFO,
               qPrintable(QString("The '%1' class has no property '%2'.")
                          .arg(className())
                          .arg(name)));

    return QpMetaProperty(property(index), *this);
}

QString QpMetaObject::tableName() const
{
    return QString(className()).toLower();
}

QList<QpMetaProperty> QpMetaObject::simpleProperties() const
{
    QList<QpMetaProperty> result;

    int count = propertyCount();
    for (int i=1; i < count; ++i) { // start at 1 because 0 is "objectName"
        QpMetaProperty metaProperty(property(i), *this);

        if(metaProperty.isStored()
                && !metaProperty.isRelationProperty()) {
            result.append(metaProperty);
        }
    }

    return result;
}

QList<QpMetaProperty> QpMetaObject::relationProperties() const
{
    QList<QpMetaProperty> result;

    int count = propertyCount();
    for (int i=1; i < count; ++i) { // start at 1 because 0 is "objectName"
        QpMetaProperty metaProperty(property(i), *this);

        if(metaProperty.isStored()
                && metaProperty.isRelationProperty()) {
            result.append(metaProperty);
        }
    }

    return result;
}

QString QpMetaObject::sqlFilter() const
{
    return classInformation(QPERSISTENCE_SQLFILTER, QString());
}

QString QpMetaObject::classInformation(const QString &name, const QString &defaultValue) const
{
    int index = indexOfClassInfo(name.toLatin1());

    if (index < 0)
        return defaultValue;

    return QLatin1String(classInfo(index).value());
}

QString QpMetaObject::classInformation(const QString &informationName, bool assertNotEmpty) const
{
    QString value = classInformation(informationName, QString());

    if(assertNotEmpty) {
        Q_ASSERT_X(!value.isEmpty(),
                   Q_FUNC_INFO,
                   qPrintable(QString("The %1 class does not define a %2.")
                              .arg(className())
                              .arg(informationName)));
    }

    return value;
}


