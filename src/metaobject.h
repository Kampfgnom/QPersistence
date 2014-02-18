#ifndef QPERSISTENCE_METAOBJECT_H
#define QPERSISTENCE_METAOBJECT_H

#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVariant>

template<class K, class V>
class QHash;
class QVariant;

class QpMetaProperty;

class QpMetaObjectPrivate;
class QpMetaObject
{
public:
    static QpMetaObject forObject(const QObject *object);
    static QpMetaObject forObject(QSharedPointer<QObject> object);
    static QpMetaObject forClassName(const QString &className);
    static QList<QpMetaObject> registeredMetaObjects();

    QpMetaObject();
    ~QpMetaObject();
    QpMetaObject(const QpMetaObject &other);
    QpMetaObject &operator = (const QpMetaObject &other);

    QMetaObject metaObject() const;

    QString className() const;

    bool isValid() const;

    QString classInformation(const QString &informationName, bool assertNotEmpty = false) const;
    QString classInformation(const QString &informationName, const QString &defaultValue) const;

    QpMetaProperty metaProperty(const QString &name) const;

    QString tableName() const;

    QList<QpMetaProperty> metaProperties() const;
    QList<QpMetaProperty> simpleProperties() const;
    QList<QpMetaProperty> relationProperties() const;

    QString sqlFilter() const;

private:
    friend class QpDaoBase;
    static QpMetaObject registerMetaObject(const QMetaObject &metaObject);
    explicit QpMetaObject(const QMetaObject &metaObject);

    void initProperties() const;
    QExplicitlySharedDataPointer<QpMetaObjectPrivate> data;
};

#endif // QPERSISTENCE_METAOBJECT_H
