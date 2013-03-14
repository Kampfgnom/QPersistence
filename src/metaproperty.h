#ifndef QPERSISTENCE_METAPROPERTY_H
#define QPERSISTENCE_METAPROPERTY_H

#include <QtCore/QMetaProperty>

#include <QtCore/QSharedDataPointer>

struct QMetaObject;
class QPersistenceMetaObject;

class QPersistenceMetaPropertyPrivate;
class QPersistenceMetaProperty : public QMetaProperty
{
public:
    enum Cardinality {
        NoCardinality,
        ToOneCardinality,
        ToManyCardinality,
        OneToOneCardinality,
        OneToManyCardinality,
        ManyToOneCardinality,
        ManyToManyCardinality
    };

    explicit QPersistenceMetaProperty(const QString &propertyName, const QPersistenceMetaObject &metaObject);
    explicit QPersistenceMetaProperty(const QMetaProperty &property, const QPersistenceMetaObject &metaObject);
    virtual ~QPersistenceMetaProperty();
    QPersistenceMetaProperty(const QPersistenceMetaProperty &other);
    QPersistenceMetaProperty &operator = (const QPersistenceMetaProperty &other);

    QPersistenceMetaObject metaObject() const;

    QString columnName() const;
    bool isAutoIncremented() const;
    bool isReadOnly() const;
    bool isPrimaryKey() const;

    // Relations
    bool isRelationProperty() const;
    bool isToOneRelationProperty() const;
    bool isToManyRelationProperty() const;
    Cardinality cardinality() const;

    QString reverseClassName() const;
    QPersistenceMetaObject reverseMetaObject() const;
    QString reverseRelationName() const;
    QPersistenceMetaProperty reverseRelation() const;

    QString tableName() const;
    QVariant::Type foreignKeyType() const;

    bool write(QObject *obj, const QVariant &value) const;

private:
    QSharedDataPointer<QPersistenceMetaPropertyPrivate> d;
};

#endif // QPERSISTENCE_METAPROPERTY_H
