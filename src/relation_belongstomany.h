#ifndef QPERSISTENCE_RELATION_BELONGSTOMANY_H
#define QPERSISTENCE_RELATION_BELONGSTOMANY_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QMetaMethod>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "qpersistence.h"

class QpBelongsToManyData;
class QpBelongsToManyBase
{
public:
    QpBelongsToManyBase(const QString &name, QObject *parent);
    virtual ~QpBelongsToManyBase();

    QList<QSharedPointer<QObject> > objects() const;
    void add(QSharedPointer<QObject> object);
    void remove(QSharedPointer<QObject> object);
    void setObjects(const QList<QSharedPointer<QObject> > &objects) const;

    bool operator ==(const QList<QSharedPointer<QObject> > &objects) const;

private:
    QExplicitlySharedDataPointer<QpBelongsToManyData> data;
};

template<class T>
class QpBelongsToMany : public QpBelongsToManyBase
{
public:
    explicit QpBelongsToMany(const QString &name, QObject *parent) : QpBelongsToManyBase(name, parent) {
    }
    virtual ~QpBelongsToMany() {
    }

    void add(QSharedPointer<T> object) {
        QpBelongsToManyBase::add(qSharedPointerCast<QObject>(object));
    }
    void remove(QSharedPointer<T> object) {
        QpBelongsToManyBase::remove(qSharedPointerCast<QObject>(object));
    }

    operator QList<QSharedPointer<T> > () const { return Qp::castList<T>(QpBelongsToManyBase::objects()); }
    QpBelongsToMany &operator=(const QList<QSharedPointer<T> > objects) {
        QpBelongsToManyBase::setObjects(Qp::castList<QObject>(objects)); return *this;
    }
    bool operator ==(const QList<QSharedPointer<T> > &objects) const {
        return QpBelongsToManyBase::operator ==(Qp::castList<QObject>(objects));
    }
};

#endif // QPERSISTENCE_RELATION_BELONGSTOMANY_H
