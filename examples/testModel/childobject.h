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

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObjectOneToOne",
                "reverserelation=childObjectOneToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObjectOneToMany",
                "reverserelation=childObjectsOneToMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObjectsManyToMany",
                "reverserelation=childObjectsManyToMany")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:belongsToOne",
                "reverserelation=hasOne")

public:
    explicit ChildObject(QObject *parent = 0);
    ~ChildObject();

    int someInt() const;
    void setSomeInt(int arg);

    QSharedPointer<ParentObject> parentObjectOneToOne() const;
    QSharedPointer<ParentObject> parentObjectOneToMany() const;
    QList<QSharedPointer<ParentObject> > parentObjectsManyToMany() const;

    QSharedPointer<ParentObject> belongsToOne() const;

public slots:
    void setBelongsToOne(QSharedPointer<ParentObject> arg);

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
};

#endif // CHILDOBJECT_H
