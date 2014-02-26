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
    Q_PROPERTY(QDateTime date READ date WRITE setDate)
    Q_PROPERTY(QSharedPointer<TestNameSpace::ChildObject> childObjectOneToOne READ childObjectOneToOne WRITE setChildObjectOneToOne)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > childObjectsOneToMany READ childObjectsOneToMany WRITE setChildObjectsOneToMany)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > childObjectsManyToMany READ childObjectsManyToMany WRITE setChildObjectsManyToMany)

    Q_PROPERTY(QSharedPointer<TestNameSpace::ChildObject> hasOne READ hasOne WRITE setHasOne)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > hasMany READ hasMany WRITE setHasMany)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ChildObject> > hasManyMany READ hasManyMany WRITE setHasManyMany)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectOneToOne",
                "reverserelation=parentObjectOneToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectsOneToMany",
                "reverserelation=parentObjectOneToMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectsManyToMany",
                "reverserelation=parentObjectsManyToMany")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hasOne",
                "reverserelation=belongsToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hasMany",
                "reverserelation=belongsToOneMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hasManyMany",
                "reverserelation=belongsToManyMany")

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
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ParentObject::TestOptions)

END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

}

#endif // PARENTOBJECT_H
