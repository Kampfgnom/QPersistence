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
    Q_PROPERTY(QSharedPointer<ChildObject> childObjectOneToOne READ childObjectOneToOne WRITE setChildObjectOneToOne)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjectsOneToMany READ childObjectsOneToMany WRITE setChildObjectsOneToMany)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjectsManyToMany READ childObjectsManyToMany WRITE setChildObjectsManyToMany)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectOneToOne",
                "reverserelation=parentObjectOneToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectsOneToMany",
                "reverserelation=parentObjectOneToMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectsManyToMany",
                "reverserelation=parentObjectsManyToMany")

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

private:
    void setChildObjectsOneToMany(QList<QSharedPointer<ChildObject> > arg);
    void setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg);

    QString m_astring;
    QpStrongRelation<ChildObject> m_childObjectOneToOne;
    QpStrongRelation<ChildObject> m_childObjectsOneToMany;
    QpStrongRelation<ChildObject> m_childObjectsManyToMany;
};

#endif // PARENTOBJECT_H
