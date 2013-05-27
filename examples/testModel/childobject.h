#ifndef CHILDOBJECT_H
#define CHILDOBJECT_H

#include <QObject>

#include <QSharedPointer>

class ParentObject;

class ChildObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int someInt READ someInt WRITE setSomeInt)
    Q_PROPERTY(QSharedPointer<ParentObject> parentObject READ parentObject)
    Q_PROPERTY(QSharedPointer<ParentObject> parentObject2 READ parentObject2)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObject",
                "reverserelation=childObject")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:parentObject2",
                "reverserelation=childObjects")


public:
    explicit ChildObject(QObject *parent = 0);
    ~ChildObject();

    int someInt() const;
    void setSomeInt(int arg);

    QSharedPointer<ParentObject> parentObject() const;
    QSharedPointer<ParentObject> parentObject2() const;

private:
    int m_someInt;
    mutable QWeakPointer<ParentObject> m_parentObject;
    mutable QWeakPointer<ParentObject> m_parentObject2;

    friend class ParentObject;
    void setParentObject(const QSharedPointer<ParentObject> &parentObject);
    void setParentObject2(const QSharedPointer<ParentObject> &parentObject);
};

#endif // CHILDOBJECT_H
