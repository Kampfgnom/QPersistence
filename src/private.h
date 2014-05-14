#ifndef QPERSISTENCE_PRIVATE_H
#define QPERSISTENCE_PRIVATE_H

#include "dataaccessobject.h"
#include "relationresolver.h"
#include "conversion.h"

namespace Qp {

namespace Private {

QObject *GlobalGuard();
void enableSharedFromThis(QSharedPointer<QObject> object);
QSharedPointer<QObject> sharedFrom(const QObject *object);

int primaryKey(QObject *object);
void setPrimaryKey(QObject *object, int key);

bool isDeleted(QObject *object);
void markAsDeleted(QObject *object);
void undelete(QObject *object);

template<class T> QList<QSharedPointer<T> > makeListStrong(const QList<QWeakPointer<T> >& list, bool *ok = 0);
template<class T> QList<QWeakPointer<T> > makeListWeak(const QList<QSharedPointer<T> >& list);
template<class T> QSharedPointer<T> resolveToOneRelation(const QString &name, const QObject *object);
template<class T> QList<QSharedPointer<T> > resolveToManyRelation(const QString &name, const QObject *object);

/*
 * Implementation:
 */
template<class T>
QList<QSharedPointer<T> > makeListStrong(const QList<QWeakPointer<T> >& list, bool *ok)
{
    QList<QSharedPointer<T> > result;
    result.reserve(list.size());
    if (ok) *ok = true;
    foreach(QWeakPointer<T> s, list) {
        QSharedPointer<T> p = s.toStrongRef();
        if (ok && !p) *ok = false;
        result.append(p);
    }
    return result;
}

template<class T>
QList<QWeakPointer<T> > makeListWeak(const QList<QSharedPointer<T> >& list)
{
    QList<QWeakPointer<T> > result;
    foreach(QSharedPointer<T> s, list) result.append(s.toWeakRef());
    return result;
}

template<class T>
QSharedPointer<T> resolveToOneRelation(const QString &name, const QObject *object)
{
    return qSharedPointerCast<T>(QpRelationResolver::resolveToOneRelation(name, object));
}

template<class T>
QList<QSharedPointer<T> > resolveToManyRelation(const QString &name, const QObject *object)
{
    return Qp::castList<T>(QpRelationResolver::resolveToOneRelation(name, object));
}

} // namespace Private

} // namespace Qp

#endif // QPERSISTENCE_PRIVATE_H

