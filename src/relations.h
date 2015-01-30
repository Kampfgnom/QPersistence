#ifndef QPERSISTENCE_RELATIONBASE_H
#define QPERSISTENCE_RELATIONBASE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include <QPersistence/private.h>

class QpRelationBasePrivate;
class QpRelationBase
{
    Q_DECLARE_PRIVATE(QpRelationBase)

protected:
    QpRelationBase(QpRelationBasePrivate &dd, const QString &name, QObject *owner);
    virtual ~QpRelationBase();

    void setStrong(bool strong);

    QScopedPointer<QpRelationBasePrivate> d_ptr;
};

class QpRelationToManyBasePrivate;
class QpRelationToManyBase : public QpRelationBase
{
    Q_DECLARE_PRIVATE(QpRelationToManyBase)

public:
    QpRelationToManyBase(const QString &name, QObject *owner);
    ~QpRelationToManyBase();

    QList<QSharedPointer<QObject> > objects() const;
    void add(QSharedPointer<QObject> object);
    void remove(QSharedPointer<QObject> object);
    void setObjects(const QList<QSharedPointer<QObject> > &objects);

    bool isResolved() const;
};

class QpRelationToOneBasePrivate;
class QpRelationToOneBase : public QpRelationBase
{
    Q_DECLARE_PRIVATE(QpRelationToOneBase)

public:
    QpRelationToOneBase(const QString &name, QObject *owner);
    ~QpRelationToOneBase();
    bool operator ==(const QSharedPointer<QObject> &object) const;
    QSharedPointer<QObject> object() const;
    void setObject(const QSharedPointer<QObject> newObject);
};

template<class T>
class QpBelongsToOne : public QpRelationToOneBase
{
public:
    explicit QpBelongsToOne(const QString &name, QObject *parent) : QpRelationToOneBase(name, parent) {
    }
    virtual ~QpBelongsToOne() {
    }

    operator QSharedPointer<T> () const { return qSharedPointerCast<T>(QpRelationToOneBase::object()); }
    QpBelongsToOne &operator=(const QSharedPointer<T> object) {
        QpRelationToOneBase::setObject(qSharedPointerCast<QObject>(object)); return *this;
    }
    bool operator ==(const QSharedPointer<T> &object) const {
        return QpRelationToOneBase::operator ==(object);
    }
    bool operator !=(const QSharedPointer<T> &object) {
        return !operator ==(object);
    }
};

template<class T>
class QpHasOne : public QpRelationToOneBase
{
public:
    explicit QpHasOne(const QString &name, QObject *parent) : QpRelationToOneBase(name, parent) {
        setStrong(true);
    }
    virtual ~QpHasOne() {
    }

    operator QSharedPointer<T> () const { return qSharedPointerCast<T>(QpRelationToOneBase::object()); }
    QpHasOne &operator=(const QSharedPointer<T> object) {
        QpRelationToOneBase::setObject(qSharedPointerCast<QObject>(object)); return *this;
    }
    bool operator ==(const QSharedPointer<T> &object) const {
        return QpRelationToOneBase::operator ==(object);
    }
    bool operator !=(const QSharedPointer<T> &object) {
        return !operator ==(object);
    }
};

template<class T>
class QpBelongsToMany : public QpRelationToManyBase
{
public:
    explicit QpBelongsToMany(const QString &name, QObject *parent) : QpRelationToManyBase(name, parent) {
    }
    virtual ~QpBelongsToMany() {
    }

    void add(QSharedPointer<T> object) {
        QpRelationToManyBase::add(qSharedPointerCast<QObject>(object));
    }
    void remove(QSharedPointer<T> object) {
        QpRelationToManyBase::remove(qSharedPointerCast<QObject>(object));
    }

    operator QList<QSharedPointer<T> > () const { return Qp::castList<T>(QpRelationToManyBase::objects()); }
    QpBelongsToMany &operator=(const QList<QSharedPointer<T> > objects) {
        QpRelationToManyBase::setObjects(Qp::castList<QObject>(objects)); return *this;
    }
};

template<class T>
class QpHasMany : public QpRelationToManyBase
{
public:
    explicit QpHasMany(const QString &name, QObject *parent) : QpRelationToManyBase(name, parent) {
        setStrong(true);
    }
    virtual ~QpHasMany() {
    }

    void add(QSharedPointer<T> object) {
        QpRelationToManyBase::add(qSharedPointerCast<QObject>(object));
    }
    void remove(QSharedPointer<T> object) {
        QpRelationToManyBase::remove(qSharedPointerCast<QObject>(object));
    }

    operator QList<QSharedPointer<T> > () const { return Qp::castList<T>(QpRelationToManyBase::objects()); }
    QpHasMany &operator=(const QList<QSharedPointer<T> > objects) {
        QpRelationToManyBase::setObjects(Qp::castList<QObject>(objects)); return *this;
    }
};


#ifndef QpRelation
#define QpRelation(Address) \
    "" #Address, this
#endif

#ifndef QpRelationName
#define QpRelationName(Address) \
    "" #Address
#endif

#endif // QPERSISTENCE_RELATIONBASE_H
