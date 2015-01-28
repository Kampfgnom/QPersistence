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
#include "condition.h"
#include "datasource.h"

class QSqlQuery;
class QpCache;
class QpStorage;
class QpDatasourceResult;

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
    QList<QSharedPointer<QObject> > readAllObjects(const QpDatasourceResult &datasourceResult) const;
    QSharedPointer<QObject> readObject(int id) const;
    QSharedPointer<QObject> createObject();
    Qp::UpdateResult updateObject(QSharedPointer<QObject> object);
    bool removeObject(QSharedPointer<QObject> object);
    bool markAsDeleted(QSharedPointer<QObject> object);
    bool undelete(QSharedPointer<QObject> object);
    enum SynchronizeMode { NormalMode, IgnoreRevision };
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
    explicit QpDataAccessObjectBase(const QMetaObject &metaObject,
                       QpStorage *parent = 0);

    virtual QObject *createInstance() const = 0;

private:
    QSharedDataPointer<QpDataAccessObjectBaseData> data;

    void unlinkRelations(QSharedPointer<QObject> object) const;

    QSharedPointer<QObject> setupSharedObject(QObject *object, int id) const;

    Qp::SynchronizeResult sync(QSharedPointer<QObject> object);
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
