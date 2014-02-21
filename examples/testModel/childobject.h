#ifndef CHILDOBJECT_H
#define CHILDOBJECT_H

#include <QPersistence.h>

#include <QObject>

#include <QSharedPointer>

class ParentObject;

class ChildObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int someInt READ someInt WRITE setSomeInt)
    Q_PROPERTY(QSharedPointer<ParentObject> parentObjectOneToOne READ parentObjectOneToOne)
    Q_PROPERTY(QSharedPointer<ParentObject> parentObjectOneToMany READ parentObjectOneToMany)
    Q_PROPERTY(QList<QSharedPointer<ParentObject> > parentObjectsManyToMany READ parentObjectsManyToMany WRITE setParentObjectsManyToMany)

    Q_PROPERTY(QSharedPointer<ParentObject> belongsToOne READ belongsToOne WRITE setBelongsToOne)
    Q_PROPERTY(QSharedPointer<ParentObject> belongsToOneMany READ belongsToOneMany WRITE setBelongsToOneMany)
    Q_PROPERTY(QList<QSharedPointer<ParentObject>> belongsToManyMany READ belongsToManyMany WRITE setBelongsToManyMany)

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

private slots:
    void setBelongsToManyMany(QList<QSharedPointer<ParentObject>> arg);
    void addBelongsToManyMany(QSharedPointer<ParentObject> arg);
    void removeBelongsToManyMany(QSharedPointer<ParentObject> arg);

private:
    int m_someInt;
    QpWeakRelation<ParentObject> m_parentObjectOneToOne;
    QpWeakRelation<ParentObject> m_parentObjectOneToMany;
    QpWeakRelation<ParentObject> m_parentObjectsManyToMany;

    friend class ParentObject;
    void setParentObjectOneToOne(const QSharedPointer<ParentObject> &parentObjectOneToOne);
    void setParentObjectOneToMany(const QSharedPointer<ParentObject> &parentObjectOneToOne);
    void setParentObjectsManyToMany(QList<QSharedPointer<ParentObject> > arg);

    void addParentObjectManyToMany(QSharedPointer<ParentObject> arg);
    void removeParentObjectManyToMany(QSharedPointer<ParentObject> arg);

    QpBelongsToOne<ParentObject> m_belongsToOne;
    QpBelongsToOne<ParentObject> m_belongsToOneMany;
    QpBelongsToMany<ParentObject> m_belongsToManyMany;
};

#endif // CHILDOBJECT_H
