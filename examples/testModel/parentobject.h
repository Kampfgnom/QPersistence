#ifndef PARENTOBJECT_H
#define PARENTOBJECT_H

#include <QPersistence.h>

#include <QObject>

#include <QSharedPointer>

class ChildObject;

class ParentObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString aString READ aString WRITE setAString)
    Q_PROPERTY(int counter READ counter WRITE setCounter)
    Q_PROPERTY(QSharedPointer<ChildObject> childObjectOneToOne READ childObjectOneToOne WRITE setChildObjectOneToOne)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjectsOneToMany READ childObjectsOneToMany WRITE setChildObjectsOneToMany)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjectsManyToMany READ childObjectsManyToMany WRITE setChildObjectsManyToMany)

    Q_PROPERTY(QSharedPointer<ChildObject> hasOne READ hasOne WRITE setHasOne)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > hasMany READ hasMany WRITE setHasMany)

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

public:
    explicit ParentObject(QObject *parent = 0);
    ~ParentObject();

    QString aString() const;
    void setAString(const QString &value);

    QSharedPointer<ChildObject> childObjectOneToOne() const;
    void setChildObjectOneToOne(QSharedPointer<ChildObject> object);

    QList<QSharedPointer<ChildObject> > childObjectsOneToMany() const;
    void addChildObjectOneToMany(QSharedPointer<ChildObject> child);
    void removeChildObjectOneToMany(QSharedPointer<ChildObject> child);

    QList<QSharedPointer<ChildObject> > childObjectsManyToMany() const;
    void addChildObjectManyToMany(QSharedPointer<ChildObject> arg);
    void removeChildObjectManyToMany(QSharedPointer<ChildObject> child);

    int counter() const;
    void increaseCounter();

    QSharedPointer<ChildObject> hasOne() const;
    void setHasOne(QSharedPointer<ChildObject> arg);

    QList<QSharedPointer<ChildObject> > hasMany() const;
    void setHasMany(QList<QSharedPointer<ChildObject> > arg);
    void addHasMany(QSharedPointer<ChildObject> arg);
    void removeHasMany(QSharedPointer<ChildObject> arg);


private:
    void setCounter(int arg);
    void setChildObjectsOneToMany(QList<QSharedPointer<ChildObject> > arg);
    void setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg);

    QString m_astring;
    QpStrongRelation<ChildObject> m_childObjectOneToOne;
    QpStrongRelation<ChildObject> m_childObjectsOneToMany;
    QpStrongRelation<ChildObject> m_childObjectsManyToMany;
    int m_counter;

    QpHasOne<ChildObject> m_hasOne;
    QpHasMany<ChildObject> m_hasMany;
};

#endif // PARENTOBJECT_H
