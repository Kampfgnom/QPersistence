#ifndef QPERSISTENCE_H
#define QPERSISTENCE_H

#include <QtSql/QSqlDatabase>

#include "dataaccessobject.h"

class QpMetaObject;

namespace Qp {

void setDatabase(const QSqlDatabase &database);
QSqlDatabase database();
void adjustDatabaseSchema();
void createCleanSchema();
QpError lastError();

void setSqlDebugEnabled(bool enable);
void startBulkDatabaseQueries();
void commitBulkDatabaseQueries();

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
template<typename T> QList<T> reversed( const QList<T> & in );
template<class Source, class Target>
QList<Target> castList(const QList<Source>& list);
template<class T, class O>
QList<QSharedPointer<T> > castList(const QList<QSharedPointer<O> >& list);

} // namespace Qp

#include "qpersistence_impl.cpp"

#define QPERSISTENCE_PROPERTYMETADATA "QPERSISTENCE_PROPERTYMETADATA"
#define QPERSISTENCE_SQLFILTER "QPERSISTENCE_SQLFILTER"
#define QPERSISTENCE_PROPERTYMETADATA_REVERSERELATION "reverserelation"


#endif // QPERSISTENCE_H
