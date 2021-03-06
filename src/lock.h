#ifndef QPERSISTENCE_LOCK_H
#define QPERSISTENCE_LOCK_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QHash>
#include <QtCore/QVariant>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#ifndef QP_NO_LOCKS
namespace Qp {
void enableLocks();
}
#endif

class QpError;
class QpSqlQuery;
class QpStorage;

class QpLockData;
class QpLock
{
public:
    QpLock();

#ifndef QP_NO_LOCKS
    enum Status {
        UnkownStatus,
        UnlockedLocalLock,
        UnlockedRemoteLock,
        UnlockedRemotely,
        LockedRemotely,
        LockedLocally,
        NotLocked,
        DatabaseError
    };

    static bool isLocked(QSharedPointer<QObject> object);
    static QpLock lockStatus(QpStorage *storage, QSharedPointer<QObject> object);
    static QpLock tryLock(QpStorage *storage, QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation);
    static QpLock unlock(QpStorage *storage, QSharedPointer<QObject> object);

    QpLock(const QpLock &);
    QpLock &operator=(const QpLock &);
    ~QpLock();

    Status status() const;
    QpError error() const;
    QSharedPointer<QObject> object() const;
    QVariant additionalInformation(const QString &name) const;

private:
    friend class QpLockData;
    explicit QpLock(const QpError &error);

    QExplicitlySharedDataPointer<QpLockData> data;
#endif
};

#endif // QPERSISTENCE_LOCK_H
