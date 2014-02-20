#ifndef LOCK_H
#define LOCK_H


#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QSharedPointer>

namespace Qp {
void enableLocks();
}

class QpError;
class QpSqlQuery;

class QpLockData;
class QpLock
{
public:
    QpLock();

#ifndef QP_NO_LOCKS
    enum Status {
        UnkownStatus,
        Unlocked,
        LockedRemotely,
        LockedLocally,
        DatabaseError
    };

    static void enableLocks();
    static bool isLocksEnabled();

    static QpLock isLocked(QSharedPointer<QObject> object);
    static QpLock tryLock(QSharedPointer<QObject> object);
    static QpLock unlock(QSharedPointer<QObject> object);

    QpLock(const QpLock &);
    QpLock &operator=(const QpLock &);
    ~QpLock();

    Status status() const;
    QpError error() const;
    QSharedPointer<QObject> object() const;

private:
    friend class QpLockData;
    explicit QpLock(const QpError &error);

    QExplicitlySharedDataPointer<QpLockData> data;
#endif
};

#endif // LOCK_H
