#ifndef QPERSISTENCE_H
#define QPERSISTENCE_H

#include <QtSql/QSqlDatabase>

#include <QPersistencePersistentDataAccessObject.h>

class QpMetaObject;

namespace Qp {

void setDatabase(const QSqlDatabase &database);
QSqlDatabase database();
void adjustDatabaseSchema();
void createCleanSchema();

template<class T> int primaryKey(QSharedPointer<T> object);
template<class T> void registerClass();
template<class T> QpDao<T> *dataAccessObject();
template<class T> QSharedPointer<T> read(int id);
template<class T> QList<QSharedPointer<T> > readAll();
template<class T> int count();
template<class T> QSharedPointer<T> create();
template<class T> bool update(QSharedPointer<T> object);
template<class T> bool remove(QSharedPointer<T> object);

template<class K, class V> void registerMappableTypes();
template<class T> void registerSetType();
template<class T> QSharedPointer<T> sharedFrom(const QObject *object);
template<class T> QSharedPointer<T> resolveToOneRelation(const QString &name, const QObject *object);
template<class T> QList<QSharedPointer<T> > resolveToManyRelation(const QString &name, const QObject *object);


template<class T> QList<QSharedPointer<T> > makeListStrong(const QList<QWeakPointer<T> >& list, bool *ok = 0);
template<class T> QList<QWeakPointer<T> > makeListWeak(const QList<QSharedPointer<T> >& list);

template <typename T> QList<T> reversed( const QList<T> & in );

} // namespace Qp

#include "qpersistence_impl.cpp"

#define QPERSISTENCE_PROPERTYMETADATA "QPERSISTENCE_PROPERTYMETADATA"
#define QPERSISTENCE_SQLFILTER "QPERSISTENCE_SQLFILTER"
#define QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION "reverserelation"


#endif // QPERSISTENCE_H
