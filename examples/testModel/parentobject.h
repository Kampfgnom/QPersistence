#ifndef PARENTOBJECT_H
#define PARENTOBJECT_H

#include <QPersistence.h>

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QSharedPointer>

namespace TestNameSpace {

class ChildObject;

class ParentObject : public QObject
{
    Q_OBJECT
    Q_ENUMS(TestEnum)
    Q_FLAGS(TestOptions)

    Q_PROPERTY(QString aString READ aString WRITE setAString)
    Q_PROPERTY(TestEnum testEnum READ testEnum WRITE setTestEnum)
    Q_PROPERTY(TestOptions testOptions READ testOptions WRITE setTestOptions)
    Q_PROPERTY(int counter READ counter WRITE setCounter)
    Q_PROPERTY(int customColumn READ customColumn WRITE setCustomColumn)
    Q_PROPERTY(int indexed READ indexed WRITE setIndexed)
    Q_PROPERTY(QDateTime date READ date WRITE setDate)
    Q_PROPERTY(QSharedPointer<TestNameSpace::ChildObject> childObjectOneToOne READ childObjectOneToOne WRITE setChildObjectOneToOne)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > childObjectsOneToMany READ childObjectsOneToMany WRITE setChildObjectsOneToMany)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > childObjectsManyToMany READ childObjectsManyToMany WRITE setChildObjectsManyToMany)

    Q_PROPERTY(QSharedPointer<TestNameSpace::ChildObject> hasOne READ hasOne WRITE setHasOne)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > hasMany READ hasMany WRITE setHasMany)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > hasManyMany READ hasManyMany WRITE setHasManyMany)

    Q_PROPERTY(int calculatedIntDependency READ calculatedIntDependency WRITE setCalculatedIntDependency NOTIFY calculatedIntDependencyChanged)
    Q_PROPERTY(int calculatedInt READ calculatedInt WRITE setCalculatedInt NOTIFY calculatedIntChanged STORED false)
    Q_PROPERTY(int calculatedInt2 READ calculatedInt2 WRITE setCalculatedInt2 NOTIFY calculatedInt2Changed STORED false)

    Q_PROPERTY(int calculatedFromHasOne READ calculatedFromHasOne WRITE setCalculatedFromHasOne NOTIFY calculatedFromHasOneChanged STORED false)
    Q_PROPERTY(int calculatedFromHasMany READ calculatedFromHasMany WRITE setCalculatedFromHasMany NOTIFY calculatedFromHasManyChanged STORED false)
    Q_PROPERTY(int calculatedFromHasManyMany READ calculatedFromHasManyMany WRITE setCalculatedFromHasManyMany NOTIFY calculatedFromHasManyManyChanged STORED false)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectOneToOne", "reverserelation=parentObjectOneToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectsOneToMany", "reverserelation=parentObjectOneToMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectsManyToMany", "reverserelation=parentObjectsManyToMany")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hasOne", "reverserelation=belongsToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hasMany", "reverserelation=belongsToOneMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hasManyMany", "reverserelation=belongsToManyMany")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:customColumn", "columnDefinition=INTEGER NOT NULL DEFAULT 5;")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:calculatedInt", "depends=this.calculatedIntDependencyChanged(int)")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:calculatedInt2", "depends=this.calculatedIntChanged(int)")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:calculatedFromHasOne", "depends=hasOne.calculatedIntDependencyChanged(int)")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:calculatedFromHasMany", "depends=hasMany.calculatedIntDependencyChanged(int)")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:calculatedFromHasManyMany", "depends=hasManyMany.calculatedIntDependencyChanged(int)")

#ifdef QP_FOR_MYSQL
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:indexed",
                "columnDefinition=INTEGER NULL;"
                "key=UNIQUE KEY")
#elif QP_FOR_SQLITE
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:indexed",
                "columnDefinition=INTEGER NULL;"
                "key=UNIQUE ")
#endif
public:
    enum TestEnum {
        NoValue,
        Value1,
        Value2,
        InitialValue,
        ExplicitValue = 13,
        ValueAfterExplicitValue
    };

    enum TestOption {
        UnknownOption = 0x0,
        Option1 = 0x1,
        Option2 = 0x2,
        Option3 = 0x4,
        InitialOption = 0x8,
        CombinedOption = Option1 | Option3
    };
    Q_DECLARE_FLAGS(TestOptions, TestOption)

    explicit ParentObject(QObject *parent = 0);
    ~ParentObject();

    QString aString() const;
    void setAString(const QString &value);

    QSharedPointer<ChildObject> childObjectOneToOne() const;
    QList<QSharedPointer<ChildObject> > childObjectsOneToMany() const;
    QList<QSharedPointer<ChildObject> > childObjectsManyToMany() const;
    QSharedPointer<ChildObject> hasOne() const;
    QList<QSharedPointer<ChildObject> > hasMany() const;
    QList<QSharedPointer<ChildObject> > hasManyMany() const;

    int counter() const;
    void increaseCounter();

    QDateTime date() const;
    void setDate(QDateTime arg);

    TestEnum testEnum() const;
    void setTestEnum(TestEnum arg);

    TestOptions testOptions() const;
    void setTestOptions(TestOptions arg);

    int indexed() const;

    int customColumn() const;

    int calculatedIntDependency() const;
    int calculatedInt() const;
    int calculatedInt2() const;

    int calculatedFromHasOne() const;
    int calculatedFromHasMany() const;
    int calculatedFromHasManyMany() const;

public slots:
    void setHasOne(QSharedPointer<ChildObject> arg);
    void setChildObjectOneToOne(QSharedPointer<ChildObject> object);

    void setChildObjectsOneToMany(QList<QSharedPointer<ChildObject> > arg);
    void addChildObjectsOneToMany(QSharedPointer<TestNameSpace::ChildObject> child);
    void removeChildObjectsOneToMany(QSharedPointer<TestNameSpace::ChildObject> child);

    void setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg);
    void addChildObjectsManyToMany(QSharedPointer<TestNameSpace::ChildObject> arg);
    void removeChildObjectsManyToMany(QSharedPointer<TestNameSpace::ChildObject> child);

    void setHasMany(QList<QSharedPointer<ChildObject> > arg);
    void addHasMany(QSharedPointer<TestNameSpace::ChildObject> arg);
    void removeHasMany(QSharedPointer<TestNameSpace::ChildObject> arg);

    void setHasManyMany(QList<QSharedPointer<ChildObject> > arg);
    void addHasManyMany(QSharedPointer<TestNameSpace::ChildObject> arg);
    void removeHasManyMany(QSharedPointer<TestNameSpace::ChildObject> arg);

    void setIndexed(int arg);
    void setCustomColumn(int arg);
    void setCalculatedIntDependency(int arg);

signals:
    void calculatedIntDependencyChanged(int arg) const;
    void calculatedIntChanged(int arg) const;
    void calculatedInt2Changed(int arg) const;

    void calculatedFromHasOneChanged(int arg) const;
    void calculatedFromHasManyChanged(int arg) const;
    void calculatedFromHasManyManyChanged(int arg) const;

private slots:
    void recalculateCalculatedInt() const;
    void setCalculatedInt(int arg) const;

    void recalculateCalculatedInt2() const;
    void setCalculatedInt2(int arg) const;

    void setCalculatedFromHasOne(int arg) const;
    void recalculateCalculatedFromHasOne() const;
    void setCalculatedFromHasMany(int arg) const;
    void recalculateCalculatedFromHasMany() const;
    void setCalculatedFromHasManyMany(int arg) const;
    void recalculateCalculatedFromHasManyMany() const;

private:
    void setCounter(int arg);

    QString m_astring;
    QpHasOne<ChildObject> m_childObjectOneToOne;
    QpHasMany<ChildObject> m_childObjectsOneToMany;
    QpHasMany<ChildObject> m_childObjectsManyToMany;
    int m_counter;

    QpHasOne<ChildObject> m_hasOne;
    QpHasMany<ChildObject> m_hasMany;
    QpHasMany<ChildObject> m_hasManyMany;
    QDateTime m_date;
    TestEnum m_testEnum;
    TestOptions m_testOptions;
    int m_indexed;
    int m_customColumn;

    int m_calculatedIntDependency;
    mutable int m_calculatedInt;
    mutable int m_calculatedInt2;

    static int NEXT_INDEX;
    mutable int m_calculatedFromHasOne;
    mutable int m_calculatedFromHasMany;
    mutable int m_calculatedFromHasManyMany;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ParentObject::TestOptions)

END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

}

#endif // PARENTOBJECT_H
