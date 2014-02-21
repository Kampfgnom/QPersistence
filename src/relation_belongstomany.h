#ifndef RELATION_BELONGSTOMANY_H
#define RELATION_BELONGSTOMANY_H

#include <QExplicitlySharedDataPointer>
#include <QSharedPointer>
#include <QVariant>
#include <QMetaMethod>

#include "qpersistence.h"

class QpBelongsToManyData;

class QpBelongsToManyBase
{
public:
    QpBelongsToManyBase(const QString &name, QObject *parent);
    virtual ~QpBelongsToManyBase();

    QList<QSharedPointer<QObject> > objects() const;
    void append(QSharedPointer<QObject> object);
    void remove(QSharedPointer<QObject> object);
    void setObjects(const QList<QSharedPointer<QObject> > objects) const;

private:
    QExplicitlySharedDataPointer<QpBelongsToManyData> data;
};

template<class T>
class QpBelongsToMany : public QpBelongsToManyBase
{
public:
    explicit QpBelongsToMany(const QString &name, QObject *parent) : QpBelongsToManyBase(name, parent) {}
    virtual ~QpBelongsToMany() {}

    void append(QSharedPointer<T> object) { QpBelongsToManyBase::append(qSharedPointerCast<QObject>(object)); }
    void remove(QSharedPointer<T> object) { QpBelongsToManyBase::remove(qSharedPointerCast<QObject>(object)); }

    operator QList<QSharedPointer<T> > () const { return Qp::castList<T>(QpBelongsToManyBase::objects()); }
    QpBelongsToMany &operator=(const QList<QSharedPointer<T> > objects) { QpBelongsToManyBase::setObjects(Qp::castList<QObject>(objects)); return *this; }
};

#endif // RELATION_BELONGSTOMANY_H
