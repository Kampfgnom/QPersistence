#ifndef PARENTOBJECT_H
#define PARENTOBJECT_H

#include <QObject>

#include <QSharedPointer>

class ChildObject;

class ParentObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString aString READ aString WRITE setAString)
    Q_PROPERTY(QSharedPointer<ChildObject> childObject READ childObject WRITE setChildObject)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjects READ childObjects)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObject",
                "reverserelation=parentObject")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:childObjects",
                "reverserelation=parentObject2")

public:
    explicit ParentObject(QObject *parent = 0);
    ~ParentObject();

    QString aString() const;
    void setAString(const QString &value);

    QSharedPointer<ChildObject> childObject() const;
    void setChildObject(QSharedPointer<ChildObject> object);

    QList<QSharedPointer<ChildObject> > childObjects() const;
    void addChildObject(QSharedPointer<ChildObject> object);

private:
    QString m_astring;
    QSharedPointer<ChildObject> m_childObject;

    QList<QSharedPointer<ChildObject> > m_childObjects;
};

#endif // PARENTOBJECT_H
