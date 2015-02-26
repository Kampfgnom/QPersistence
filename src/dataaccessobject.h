#ifndef QPERSISTENCE_DATAACCESSOBJECT_H
#define QPERSISTENCE_DATAACCESSOBJECT_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtSql/QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include <functional>

#include "conversion.h"
#include "metaobject.h"
#include "condition.h"
#include "datasource.h"

class QSqlQuery;
class QpCache;
class QpReply;
class QpStorage;
class QpDatasourceResult;
class QpDataTransferObjectDiff;

namespace Qp {
enum SynchronizeResult : short;
enum UpdateResult : short;
}


class QpDataAccessObjectBaseData;
class QpDataAccessObjectBase : public QObject
{
    Q_OBJECT
public:
    ~QpDataAccessObjectBase();

    QpMetaObject qpMetaObject() const;

    int count(const QpCondition &condition = QpCondition()) const;
    QList<int> allKeys(int skip = -1, int count = -1) const;
    QList<QSharedPointer<QObject> > readAllObjects(int skip = -1,
                                                   int limit = -1,
                                                   const QpCondition &condition = QpCondition(),
                                                   QList<QpDatasource::OrderField> orders = QList<QpDatasource::OrderField>()) const;
    QList<QSharedPointer<QObject> > readObjectsUpdatedAfterRevision(int revision) const;
    QList<QSharedPointer<QObject> > readAllObjects(const QList<int> primaryKeys) const;
    QSharedPointer<QObject> readObject(int id) const;
    QSharedPointer<QObject> createObject();
    Qp::UpdateResult updateObject(QSharedPointer<QObject> object);
    bool removeObject(QSharedPointer<QObject> object);
    bool markAsDeleted(QSharedPointer<QObject> object);
    bool undelete(QSharedPointer<QObject> object);
    enum SynchronizeMode { NormalMode, IgnoreRevision, RebaseMode };
    Qp::SynchronizeResult synchronizeObject(QSharedPointer<QObject> object, SynchronizeMode mode = NormalMode);
    int revisionInDatabase(QSharedPointer<QObject> object);
    bool incrementNumericColumn(QSharedPointer<QObject> object, const QString &fieldName);

#ifndef QP_NO_TIMESTAMPS
    QList<QSharedPointer<QObject> > createdSince(const QDateTime &time);
    QList<QSharedPointer<QObject> > createdSince(double time);
    QList<QSharedPointer<QObject> > updatedSince(const QDateTime &time);
    QList<QSharedPointer<QObject> > updatedSince(double time);
#endif

    QpCache cache() const;

    QpStorage *storage() const;

    void resetLastKnownSynchronization();

    QpReply *readAllObjectsAsync(int skip = -1,
                                 int limit = -1,
                                 const QpCondition &condition = QpCondition(),
                                 QList<QpDatasource::OrderField> orders = {}) const;
    QpReply *readObjectsUpdatedAfterRevisionAsync(int revision) const;

public slots:
    bool synchronizeAllObjects();
    QpReply *synchronizeAllObjectsAsync();

Q_SIGNALS:
    void objectInstanceCreated(QSharedPointer<QObject>) const;
    void objectCreated(QSharedPointer<QObject>);
    void objectMarkedAsDeleted(QSharedPointer<QObject>);
    void objectUndeleted(QSharedPointer<QObject>);
    void objectUpdated(QSharedPointer<QObject>);
    void objectRemoved(QSharedPointer<QObject>);
    void objectSynchronized(QSharedPointer<QObject>);

protected:
    explicit QpDataAccessObjectBase(const QMetaObject &metaObject,
                       QpStorage *parent = 0);

    virtual QObject *createInstance() const = 0;

private slots:
    void handleResultError();

private:
    QSharedDataPointer<QpDataAccessObjectBaseData> data;

    void unlinkRelations(QSharedPointer<QObject> object) const;
    QSharedPointer<QObject> setupSharedObject(QObject *object, int id) const;
    Qp::SynchronizeResult sync(QSharedPointer<QObject> object, SynchronizeMode mode = NormalMode);
    QList<QSharedPointer<QObject> > readObjects(QpDatasourceResult *datasourceResult) const;
    void handleCreatedObjects(const QList<QSharedPointer<QObject> > &objects);
    void handleUpdatedObjects(const QList<QSharedPointer<QObject> > &objects);

    QpReply *makeReply(QpDatasourceResult *result, std::function<void(QpDatasourceResult *, QpReply *)> handleResult) const;
};

namespace Qp {
template<class T, class Source>
QList<QSharedPointer<T> > castList(const QList<QSharedPointer<Source> >&);
}

template<class T>
class QpDataAccessObject : public QpDataAccessObjectBase
{
public:
    QSharedPointer<T> read(int id) {
        return qSharedPointerCast<T>(readObject(id));
    }
    QList<QSharedPointer<T> > readAllObjects(int skip = -1,
                                             int count = -1,
                                             const QpCondition &condition = QpCondition(),
                                             QList<QpDatasource::OrderField> orders = QList<QpDatasource::OrderField>()) const
    {
        return Qp::castList<T>(QpDataAccessObjectBase::readAllObjects(skip, count, condition, orders));
    }

protected:
    QpDataAccessObject(QpStorage *parent) :
        QpDataAccessObjectBase(T::staticMetaObject, parent)
    {
    }

    QObject *createInstance() const Q_DECL_OVERRIDE {
        return new T;
    }

private:
    friend class QpStorage;
};

#endif // QPERSISTENCE_DATAACCESSOBJECT_H
