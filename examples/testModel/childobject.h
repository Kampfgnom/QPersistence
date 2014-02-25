#ifndef CHILDOBJECT_H
#define CHILDOBJECT_H

#include <QPersistence.h>

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>

#include <QSharedPointer>

namespace TestNameSpace {

class ParentObject;

class ChildObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int someInt READ someInt WRITE setSomeInt)
    Q_PROPERTY(QSharedPointer<TestNameSpace::ParentObject> parentObjectOneToOne READ parentObjectOneToOne WRITE setParentObjectOneToOne)
    Q_PROPERTY(QSharedPointer<TestNameSpace::ParentObject> parentObjectOneToMany READ parentObjectOneToMany WRITE setParentObjectOneToMany)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ParentObject> > parentObjectsManyToMany READ parentObjectsManyToMany WRITE setParentObjectsManyToMany)

    Q_PROPERTY(QSharedPointer<TestNameSpace::ParentObject> belongsToOne READ belongsToOne WRITE setBelongsToOne)
    Q_PROPERTY(QSharedPointer<TestNameSpace::ParentObject> belongsToOneMany READ belongsToOneMany WRITE setBelongsToOneMany)
    Q_PROPERTY(QList<QSharedPointer<TestNameSpace::ParentObject>> belongsToManyMany READ belongsToManyMany WRITE setBelongsToManyMany)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObjectOneToOne",
                "reverserelation=childObjectOneToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObjectOneToMany",
                "reverserelation=childObjectsOneToMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObjectsManyToMany",
                "reverserelation=childObjectsManyToMany")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:belongsToOne",
                "reverserelation=hasOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:belongsToOneMany",
                "reverserelation=hasMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:belongsToManyMany",
                "reverserelation=hasManyMany")

public:
    explicit ChildObject(QObject *parent = 0);
    ~ChildObject();

    int someInt() const;
    void setSomeInt(int arg);

    QSharedPointer<ParentObject> parentObjectOneToOne() const;
    QSharedPointer<ParentObject> parentObjectOneToMany() const;
    QList<QSharedPointer<ParentObject> > parentObjectsManyToMany() const;

    QSharedPointer<ParentObject> belongsToOne() const;
    void setBelongsToOne(QSharedPointer<ParentObject> arg);

    QSharedPointer<ParentObject> belongsToOneMany() const;
    void setBelongsToOneMany(QSharedPointer<ParentObject> arg);

    QList<QSharedPointer<ParentObject>> belongsToManyMany() const;

public slots:
    void setBelongsToManyMany(QList<QSharedPointer<ParentObject>> arg);
    void addBelongsToManyMany(QSharedPointer<TestNameSpace::ParentObject> arg);
    void removeBelongsToManyMany(QSharedPointer<TestNameSpace::ParentObject> arg);

    void setParentObjectsManyToMany(QList<QSharedPointer<ParentObject> > arg);
    void addParentObjectsManyToMany(QSharedPointer<TestNameSpace::ParentObject> arg);
    void removeParentObjectsManyToMany(QSharedPointer<TestNameSpace::ParentObject> arg);

private:
    int m_someInt;
    QpBelongsToOne<ParentObject> m_parentObjectOneToOne;
    QpBelongsToOne<ParentObject> m_parentObjectOneToMany;
    QpBelongsToMany<ParentObject> m_parentObjectsManyToMany;

    friend class ParentObject;
    void setParentObjectOneToOne(const QSharedPointer<ParentObject> &parentObjectOneToOne);
    void setParentObjectOneToMany(const QSharedPointer<ParentObject> &parentObjectOneToOne);


    QpBelongsToOne<ParentObject> m_belongsToOne;
    QpBelongsToOne<ParentObject> m_belongsToOneMany;
    QpBelongsToMany<ParentObject> m_belongsToManyMany;
};

END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

}

#endif // CHILDOBJECT_H
