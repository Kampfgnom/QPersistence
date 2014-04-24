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

class QSqlQuery;
class QpCache;
class QpError;
class QpSqlDataAccessObjectHelper;

namespace Qp {
enum SynchronizeResult : short;
enum UpdateResult : short;
template<class T>
struct SynchronizeAllResult;
}


class QpDaoBaseData;
class QpDaoBase : public QObject
{
    Q_OBJECT
public:
    static QpDaoBase *forClass(const QMetaObject &metaObject);
    static QList<QpDaoBase *> dataAccessObjects();

    ~QpDaoBase();

    QpSqlDataAccessObjectHelper *sqlDataAccessObjectHelper() const;
    QpMetaObject qpMetaObject() const;

    int count() const;
    QList<int> allKeys(int skip = -1, int count = -1) const;
    QList<QSharedPointer<QObject> > readAllObjects(int skip = -1, int count = -1, const QpSqlCondition &condition = QpSqlCondition()) const;
    QSharedPointer<QObject> readObject(int id) const;
    QSharedPointer<QObject> createObject();
    Qp::UpdateResult updateObject(QSharedPointer<QObject> object);
    bool removeObject(QSharedPointer<QObject> object);
    bool markAsDeleted(QSharedPointer<QObject> object);
    bool undelete(QSharedPointer<QObject> object);
    enum SynchronizeMode { NormalMode, IgnoreTimes };
    Qp::SynchronizeResult synchronizeObject(QSharedPointer<QObject> object, SynchronizeMode mode = NormalMode);
    bool synchronizeAllObjects();

#ifndef QP_NO_TIMESTAMPS
    QList<QSharedPointer<QObject>> createdSince(const QDateTime &time);
    QList<QSharedPointer<QObject> > createdSince(double time);
    QList<QSharedPointer<QObject>> updatedSince(const QDateTime &time);
    QList<QSharedPointer<QObject> > updatedSince(double time);
#endif

    QpError lastError() const;
    QpCache cache() const;

Q_SIGNALS:
    void objectCreated(QSharedPointer<QObject>);
    void objectMarkedAsDeleted(QSharedPointer<QObject>);
    void objectUndeleted(QSharedPointer<QObject>);
    void objectUpdated(QSharedPointer<QObject>);
    void objectRemoved(QSharedPointer<QObject>);

protected:
    explicit QpDaoBase(const QMetaObject &metaObject,
                       QObject *parent = 0);

    virtual QObject *createInstance() const = 0;

private:
    QSharedDataPointer<QpDaoBaseData> data;

    void setLastError(const QpError &error) const;
    void resetLastError() const;

    Qp::SynchronizeResult sync(QSharedPointer<QObject> object);

    Q_DISABLE_COPY(QpDaoBase)
};

namespace Qp {
template<class T, class... Superclasses>
void registerClass();
template<class T, class Source>
QList<QSharedPointer<T> > castList(const QList<QSharedPointer<Source> >&);
}

template<class T>
class QpDao : public QpDaoBase
{
public:
    QSharedPointer<T> read(int id) { return qSharedPointerCast<T>(readObject(id)); }
    QList<QSharedPointer<T> > readAllObjects(int skip = -1, int count = -1) const
    {
        return Qp::castList<T>(QpDaoBase::readAllObjects(skip, count));
    }

protected:
    QpDao(QObject *parent) :
        QpDaoBase(T::staticMetaObject, parent)
    {}

    QObject *createInstance() const Q_DECL_OVERRIDE { return new T; }

private:
    template<class O, class... Superclasses> friend void Qp::registerClass();
    Q_DISABLE_COPY(QpDao)
};

#endif // QPERSISTENCE_DATAACCESSOBJECT_H
