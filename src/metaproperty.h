#ifndef QPERSISTENCE_METAPROPERTY_H
#define QPERSISTENCE_METAPROPERTY_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QMetaProperty>
#include <QtCore/QSharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

struct QMetaObject;
class QpMetaObject;

class QpMetaPropertyPrivate;
class QpMetaProperty
{
public:
    enum Cardinality {
        UnknownCardinality,
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

    QHash<QString, QString> attributes() const;

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
    void parseAttributes() const;

    QExplicitlySharedDataPointer<QpMetaPropertyPrivate> data;
};

#endif // QPERSISTENCE_METAPROPERTY_H
