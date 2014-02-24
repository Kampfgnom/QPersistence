#ifndef LOCK_H
#define LOCK_H

#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QHash>
#include <QtCore/QVariant>

namespace Qp {
void enableLocks();
}

class QpError;
class QpSqlQuery;

class QpLockData;
class QpLock
{
public:
    enum Status {
        UnkownStatus,
        Unlocked,
        LockedRemotely,
        LockedLocally,
        DatabaseError
    };

    static void enableLocks();
    static bool isLocksEnabled();
    static void addAdditionalInformationField(const QString &name, QVariant::Type type = QVariant::UserType);
    static QHash<QString, QVariant::Type> additionalInformationFields();

    static QpLock isLocked(QSharedPointer<QObject> object);
    static QpLock tryLock(QSharedPointer<QObject> object, QHash<QString,QVariant> additionalInformation);
    static QpLock unlock(QSharedPointer<QObject> object);

    QpLock();
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
};

#endif // LOCK_H
