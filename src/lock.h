#ifndef LOCK_H
#define LOCK_H

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

    static void enableLocks();
    static bool isLocksEnabled();
    static void addAdditionalInformationField(const QString &name, QVariant::Type type = QVariant::UserType);
    static QHash<QString, QVariant::Type> additionalInformationFields();

    static QpLock isLocked(QSharedPointer<QObject> object);
    static QpLock tryLock(QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation);
    static QpLock unlock(QSharedPointer<QObject> object);

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

#endif // LOCK_H
