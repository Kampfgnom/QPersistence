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
    Q_PROPERTY(QDateTime date READ date WRITE setDate)
    Q_PROPERTY(QSharedPointer<ChildObject> childObjectOneToOne READ childObjectOneToOne WRITE setChildObjectOneToOne)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjectsOneToMany READ childObjectsOneToMany WRITE setChildObjectsOneToMany)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > childObjectsManyToMany READ childObjectsManyToMany WRITE setChildObjectsManyToMany)

    Q_PROPERTY(QSharedPointer<ChildObject> hasOne READ hasOne WRITE setHasOne)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > hasMany READ hasMany WRITE setHasMany)
    Q_PROPERTY(QList<QSharedPointer<ChildObject> > hasManyMany READ hasManyMany WRITE setHasManyMany)

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

public slots:
    void setHasOne(QSharedPointer<ChildObject> arg);
    void setChildObjectOneToOne(QSharedPointer<ChildObject> object);

    void setChildObjectsOneToMany(QList<QSharedPointer<ChildObject> > arg);
    void addChildObjectsOneToMany(QSharedPointer<ChildObject> child);
    void removeChildObjectsOneToMany(QSharedPointer<ChildObject> child);

    void setChildObjectsManyToMany(QList<QSharedPointer<ChildObject> > arg);
    void addChildObjectsManyToMany(QSharedPointer<ChildObject> arg);
    void removeChildObjectsManyToMany(QSharedPointer<ChildObject> child);

    void setHasMany(QList<QSharedPointer<ChildObject> > arg);
    void addHasMany(QSharedPointer<ChildObject> arg);
    void removeHasMany(QSharedPointer<ChildObject> arg);

    void setHasManyMany(QList<QSharedPointer<ChildObject> > arg);
    void addHasManyMany(QSharedPointer<ChildObject> arg);
    void removeHasManyMany(QSharedPointer<ChildObject> arg);


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
};

#endif // PARENTOBJECT_H
