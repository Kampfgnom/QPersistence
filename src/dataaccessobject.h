#ifndef QPERSISTENCE_DATAACCESSOBJECT_H
#define QPERSISTENCE_DATAACCESSOBJECT_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtSql/QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


#include "conversion.h"
#include "metaobject.h"
#include "sqlcondition.h"
#include "sqlquery.h"

class QSqlQuery;
class QpCache;
class QpSqlDataAccessObjectHelper;
class QpStorage;

namespace Qp {
enum SynchronizeResult : short;
enum UpdateResult : short;
}


class QpDaoBaseData;
class QpDaoBase : public QObject
{
    Q_OBJECT
public:
    ~QpDaoBase();

    QpMetaObject qpMetaObject() const;

    int count(const QpSqlCondition &condition = QpSqlCondition()) const;
    QList<int> allKeys(int skip = -1, int count = -1) const;
    QList<QSharedPointer<QObject> > readAllObjects(int skip = -1,
                                                   int count = -1,
                                                   const QpSqlCondition &condition = QpSqlCondition(),
                                                   QList<QpSqlQuery::OrderField> orders = QList<QpSqlQuery::OrderField>()) const;
    QList<QSharedPointer<QObject> > readObjectsUpdatedAfterRevision(int revision) const;
    QList<QSharedPointer<QObject> > readAllObjects(QpSqlQuery &query) const;
    QSharedPointer<QObject> readObject(int id) const;
    QSharedPointer<QObject> createObject();
    Qp::UpdateResult updateObject(QSharedPointer<QObject> object);
    bool removeObject(QSharedPointer<QObject> object);
    bool markAsDeleted(QSharedPointer<QObject> object);
    bool undelete(QSharedPointer<QObject> object);
    enum SynchronizeMode { NormalMode, IgnoreRevision };
    Qp::SynchronizeResult synchronizeObject(QSharedPointer<QObject> object, SynchronizeMode mode = NormalMode);
    bool incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName);

    int latestRevision() const;

#ifndef QP_NO_TIMESTAMPS
    QList<QSharedPointer<QObject> > createdSince(const QDateTime &time);
    QList<QSharedPointer<QObject> > createdSince(double time);
    QList<QSharedPointer<QObject> > updatedSince(const QDateTime &time);
    QList<QSharedPointer<QObject> > updatedSince(double time);
#endif

    QpCache cache() const;

    void resetLastKnownSynchronization();

public slots:
    bool synchronizeAllObjects();

Q_SIGNALS:
    void objectInstanceCreated(QSharedPointer<QObject>) const;
    void objectCreated(QSharedPointer<QObject>);
    void objectMarkedAsDeleted(QSharedPointer<QObject>);
    void objectUndeleted(QSharedPointer<QObject>);
    void objectUpdated(QSharedPointer<QObject>);
    void objectRemoved(QSharedPointer<QObject>);
    void objectSynchronized(QSharedPointer<QObject>);

protected:
    explicit QpDaoBase(const QMetaObject &metaObject,
                       QpStorage *parent = 0);

    virtual QObject *createInstance() const = 0;

private:
    QSharedDataPointer<QpDaoBaseData> data;

    void unlinkRelations(QSharedPointer<QObject> object) const;

    QSharedPointer<QObject> setupSharedObject(QObject *object, int id) const;

    Qp::SynchronizeResult sync(QSharedPointer<QObject> object);

    Q_DISABLE_COPY(QpDaoBase)
};

namespace Qp {
template<class T, class Source>
QList<QSharedPointer<T> > castList(const QList<QSharedPointer<Source> >&);
}

template<class T>
class QpDao : public QpDaoBase
{
public:
    QSharedPointer<T> read(int id) {
        return qSharedPointerCast<T>(readObject(id));
    }
    QList<QSharedPointer<T> > readAllObjects(int skip = -1,
                                             int count = -1,
                                             const QpSqlCondition &condition = QpSqlCondition(),
                                             QList<QpSqlQuery::OrderField> orders = QList<QpSqlQuery::OrderField>()) const
    {
        return Qp::castList<T>(QpDaoBase::readAllObjects(skip, count, condition, orders));
    }

protected:
    QpDao(QpStorage *parent) :
        QpDaoBase(T::staticMetaObject, parent)
    {
    }

    QObject *createInstance() const Q_DECL_OVERRIDE {
        return new T;
    }

private:
    friend class QpStorage;
    Q_DISABLE_COPY(QpDao)
};

#endif // QPERSISTENCE_DATAACCESSOBJECT_H
