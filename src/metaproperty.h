#ifndef QPERSISTENCE_METAPROPERTY_H
#define QPERSISTENCE_METAPROPERTY_H

#include <QtCore/QMetaProperty>
#include <QtCore/QSharedDataPointer>

struct QMetaObject;
class QpMetaObject;

class QpMetaPropertyPrivate;
class QpMetaProperty
{
public:
    enum Cardinality {
        UnknownCardinality,
        NoCardinality,
        ToOneCardinality,
        ToManyCardinality,
        OneToOneCardinality,
        OneToManyCardinality,
        ManyToOneCardinality,
        ManyToManyCardinality
    };

    QpMetaProperty();
    virtual ~QpMetaProperty();
    QpMetaProperty(const QpMetaProperty &other);
    QpMetaProperty &operator = (const QpMetaProperty &other);

    QpMetaObject metaObject() const;

    QMetaProperty metaProperty() const;
    bool write(QObject *obj, const QVariant &value) const;

    QString name() const;
    QString typeName() const;
    QString columnName() const;

    bool isStored() const;
    bool isValid() const;
    QVariant::Type type() const;

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
    friend class QpMetaObject;
    explicit QpMetaProperty(const QMetaProperty &property, const QpMetaObject &metaObject);

    QString generateColumnName() const;

    QString shortName(const QString &name) const;

    QExplicitlySharedDataPointer<QpMetaPropertyPrivate> data;
};

#endif // QPERSISTENCE_METAPROPERTY_H
