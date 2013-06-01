#ifndef QPERSISTENCE_METAOBJECT_H
#define QPERSISTENCE_METAOBJECT_H

#include <QtCore/QMetaObject>

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QSharedDataPointer>

template<class K, class V>
class QHash;
class QVariant;

class QpMetaProperty;

class QpMetaObjectPrivate;
class QpMetaObject : public QMetaObject
{
public:
    QpMetaObject();
    explicit QpMetaObject(const QMetaObject &metaObject);
    ~QpMetaObject();
    QpMetaObject(const QpMetaObject &other);
    QpMetaObject &operator = (const QpMetaObject &other);

    QString classInformation(const QString &informationName, bool assertNotEmpty = false) const;
    QString classInformation(const QString &informationName, const QString &defaultValue) const;

    QpMetaProperty metaProperty(const QString &name) const;

    QString tableName() const;

    QList<QpMetaProperty> simpleProperties() const;
    QList<QpMetaProperty> relationProperties() const;

    QString sqlFilter() const;

private:
    QSharedDataPointer<QpMetaObjectPrivate> d;
};

#endif // QPERSISTENCE_METAOBJECT_H
