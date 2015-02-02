#ifndef QPERSISTENCE_METAPROPERTY_H
#define QPERSISTENCE_METAPROPERTY_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QMetaProperty>
#include <QtCore/QSharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

struct QMetaObject;
class QpMetaObject;

class QpMetaPropertyData;
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

    static QString nameFromMaybeQualifiedName(const QString &maybeQualifiedName);

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

    QHash<QString, QString> attributes() const;

    bool isStored() const;
    bool isValid() const;
    bool isLazy() const;
    bool hasAnnotation(const QString &name) const;
    QVariant::Type type() const;

    // Calculated properties
    bool isCalculated() const;
    QStringList dependencies() const;
    QMetaMethod recalculateMethod(QSharedPointer<QObject> object) const;

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

    void remove(QSharedPointer<QObject> object, QSharedPointer<QObject> related) const;
    void add(QSharedPointer<QObject> object, QSharedPointer<QObject> related) const;
    QList<QSharedPointer<QObject > > read(QSharedPointer<QObject> object) const;

    bool isRelated(QSharedPointer<QObject> left, QSharedPointer<QObject> right) const;

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

    QExplicitlySharedDataPointer<QpMetaPropertyData> data;
};

uint qHash(const QpMetaProperty &t, uint seed);
bool operator==(const QpMetaProperty &a1, const QpMetaProperty &a2);

#endif // QPERSISTENCE_METAPROPERTY_H
