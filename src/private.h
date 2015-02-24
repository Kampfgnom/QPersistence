#ifndef QPERSISTENCE_PRIVATE_H
#define QPERSISTENCE_PRIVATE_H

#include "dataaccessobject.h"
#include "conversion.h"

namespace Qp {

namespace Private {

void enableSharedFromThis(QSharedPointer<QObject> object);
QSharedPointer<QObject> sharedFrom(const QObject *object);

int primaryKey(const QObject *object);
int primaryKey(QSharedPointer<QObject> object);
QList<int> primaryKeys(const QList<QSharedPointer<QObject> > &objects);
void setPrimaryKey(QObject *object, int key);
int revisionInObject(const QObject *object);

bool isDeleted(const QObject *object);
bool isDeleted(QSharedPointer<QObject> object);
void markAsDeleted(QObject *object);
void undelete(QObject *object);

template<class T> QList<QSharedPointer<T> > makeListStrong(const QList<QWeakPointer<T> >& list, bool *ok = 0);
template<class T> QList<QWeakPointer<T> > makeListWeak(const QList<QSharedPointer<T> >& list);

/*
 * Implementation:
 */
template<class T>
QList<QSharedPointer<T> > makeListStrong(const QList<QWeakPointer<T> >& list, bool *ok)
{
    QList<QSharedPointer<T> > result;
    result.reserve(list.size());
    if (ok) *ok = true;
    foreach (QWeakPointer<T> s, list) {
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
    foreach (QSharedPointer<T> s, list) result.append(s.toWeakRef());
    return result;
}

} // namespace Private

} // namespace Qp

#endif // QPERSISTENCE_PRIVATE_H

