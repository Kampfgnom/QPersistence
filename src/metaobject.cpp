#include "metaobject.h"

#include "metaproperty.h"
#include "abstractdataaccessobject.h"
#include "qpersistence.h"

#include <QString>
#include <QDebug>
#include <QMetaClassInfo>
#include <QHash>

class QPersistenceMetaObjectPrivate : public QSharedData
{
public:
    QPersistenceMetaObjectPrivate() :
        QSharedData()
    {}
};

QPersistenceMetaObject::QPersistenceMetaObject() :
    QMetaObject(),
    d(new QPersistenceMetaObjectPrivate)
{
}

QPersistenceMetaObject::QPersistenceMetaObject(const QMetaObject &metaObject) :
    QMetaObject(metaObject),
    d(new QPersistenceMetaObjectPrivate)
{
}

QPersistenceMetaObject::~QPersistenceMetaObject()
{
}

QPersistenceMetaObject::QPersistenceMetaObject(const QPersistenceMetaObject &other) :
    QMetaObject(other),
    d(other.d)
{
}

QPersistenceMetaObject &QPersistenceMetaObject::operator =(const QPersistenceMetaObject &other)
{
    QMetaObject::operator=(other);

    if(this != &other) {
        d = other.d;
    }

    return *this;
}

bool QPersistenceMetaObject::hasMetaProperty(const QString &name) const
{
    int index = indexOfProperty(name.toLatin1());
    return index >= 0;
}

QPersistenceMetaProperty QPersistenceMetaObject::metaProperty(const QString &name) const
{
    int index = indexOfProperty(name.toLatin1());

    Q_ASSERT_X(index >= 0,
               Q_FUNC_INFO,
               qPrintable(QString("The %1 class has no property %2.")
                          .arg(className())
                          .arg(name)));

    return QPersistenceMetaProperty(property(index), *this);
}

QString QPersistenceMetaObject::tableName() const
{
    return QPersistenceMetaObject::classInformation(QPERSISTENCE_SQL_TABLENAME, QLatin1String(className()));
}

QString QPersistenceMetaObject::collectionName() const
{
    return QPersistenceMetaObject::classInformation(QPERSISTENCE_REST_COLLECTIONNAME, QLatin1String(className()));
}

QString QPersistenceMetaObject::primaryKeyPropertyName() const
{
    return QPersistenceMetaObject::classInformation(QPERSISTENCE_PRIMARYKEY, true);
}

QPersistenceMetaProperty QPersistenceMetaObject::primaryKeyProperty() const
{
    return metaProperty(primaryKeyPropertyName());
}

QList<QPersistenceMetaProperty> QPersistenceMetaObject::simpleProperties() const
{
    QList<QPersistenceMetaProperty> result;

    int count = propertyCount();
    for (int i=1; i < count; ++i) { // start at 1 because 0 is "objectName"
        QPersistenceMetaProperty metaProperty(property(i), *this);

        if(metaProperty.isStored()
                && !metaProperty.isRelationProperty()) {
            result.append(metaProperty);
        }
    }

    return result;
}

QList<QPersistenceMetaProperty> QPersistenceMetaObject::relationProperties() const
{
    QList<QPersistenceMetaProperty> result;

    int count = propertyCount();
    for (int i=1; i < count; ++i) { // start at 1 because 0 is "objectName"
        QPersistenceMetaProperty metaProperty(property(i), *this);

        if(metaProperty.isStored()
                && metaProperty.isRelationProperty()) {
            result.append(metaProperty);
        }
    }

    return result;
}

QString QPersistenceMetaObject::classInformation(const QString &name, const QString &defaultValue) const
{
    int index = indexOfClassInfo(name.toLatin1());

    if (index < 0)
        return defaultValue;

    return QLatin1String(classInfo(index).value());
}

QString QPersistenceMetaObject::classInformation(const QString &informationName, bool assertNotEmpty) const
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


