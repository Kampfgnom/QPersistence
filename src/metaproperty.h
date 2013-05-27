#ifndef QPERSISTENCE_METAPROPERTY_H
#define QPERSISTENCE_METAPROPERTY_H

#include <QtCore/QMetaProperty>

#include <QtCore/QSharedDataPointer>

struct QMetaObject;
class QpMetaObject;

class QpMetaPropertyPrivate;
class QpMetaProperty : public QMetaProperty
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

    explicit QpMetaProperty(const QString &propertyName, const QpMetaObject &metaObject);
    explicit QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject);
    virtual ~QpMetaProperty();
    QpMetaProperty(const QpMetaProperty &other);
    QpMetaProperty &operator = (const QpMetaProperty &other);

    QpMetaObject metaObject() const;

    bool write(QObject *obj, const QVariant &value) const;

    QString columnName() const;

    // Relations
    bool isRelationProperty() const;
    bool isToOneRelationProperty() const;
    bool isToManyRelationProperty() const;
    bool hasTableForeignKey() const;
    Cardinality cardinality() const;

    QString reverseClassName() const;
    QpMetaObject reverseMetaObject() const;
    QString reverseRelationName() const;
    QpMetaProperty reverseRelation() const;

    QString tableName() const;

    // Maps
    bool isMappingProperty() const;
    QString mappingFromTypeName() const;
    QString mappingToTypeName() const;

    // Sets
    bool isSetProperty() const;
    QString setType() const;

private:
    QSharedDataPointer<QpMetaPropertyPrivate> d;
};

#endif // QPERSISTENCE_METAPROPERTY_H
