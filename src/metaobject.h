#ifndef QPERSISTENCE_METAOBJECT_H
#define QPERSISTENCE_METAOBJECT_H

#include <QtCore/QMetaObject>

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QSharedDataPointer>

template<class K, class V>
class QHash;
class QVariant;

class QPersistenceMetaProperty;
class QPersistenceAbstractDataAccessObject;

namespace QPersistence {
class ConverterBase;
}

class QPersistenceMetaObjectPrivate;
class QPersistenceMetaObject : public QMetaObject
{
public:
    QPersistenceMetaObject();
    explicit QPersistenceMetaObject(const QMetaObject &metaObject);
    ~QPersistenceMetaObject();
    QPersistenceMetaObject(const QPersistenceMetaObject &other);
    QPersistenceMetaObject &operator = (const QPersistenceMetaObject &other);

    QString classInformation(const QString &informationName, bool assertNotEmpty = false) const;
    QString classInformation(const QString &informationName, const QString &defaultValue) const;

    bool hasMetaProperty(const QString &name) const;
    QPersistenceMetaProperty metaProperty(const QString &name) const;

    QString tableName() const;
    QString primaryKeyPropertyName() const;
    QPersistenceMetaProperty primaryKeyProperty() const;

    QString collectionName() const;

    QList<QPersistenceMetaProperty> simpleProperties() const;
    QList<QPersistenceMetaProperty> relationProperties() const;

private:
    QSharedDataPointer<QPersistenceMetaObjectPrivate> d;
};

#endif // QPERSISTENCE_METAOBJECT_H
