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
    Q_PROPERTY(QSharedPointer<ChildObject> childObject READ childObject WRITE setChildObject)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjects READ childObjects)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjectsManyToMany READ childObjectsManyToMany WRITE setChildObjectsManyToMany)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObject",
                "reverserelation=parentObjectOneToOne")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjects",
                "reverserelation=parentObjectOneToMany")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjectsManyToMany",
                "reverserelation=parentObjectsManyToMany")

public:
    explicit ParentObject(QObject *parent = 0);
    ~ParentObject();

    QString aString() const;
    void setAString(const QString &value);

    QSharedPointer<ChildObject> childObject() const;
    void setChildObject(QSharedPointer<ChildObject> object);

    QList<QSharedPointer<ChildObject> > childObjects() const;
    void addChildObject(QSharedPointer<ChildObject> object);

    QList<QSharedPointer<ChildObject> > childObjectsManyToMany() const;
    void addChildObjectManyToMany(QSharedPointer<ChildObject> arg);

private:
    void setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg);

    QString m_astring;
    QpStrongRelation<ChildObject> m_childObject;

    QpStrongRelation<ChildObject> m_childObjects;
    QpStrongRelation<ChildObject> m_childObjectsManyToMany;
};

#endif // PARENTOBJECT_H
