#ifndef LOCK_H
#define LOCK_H

#include <QObject>

#include <QSharedDataPointer>

namespace Qp {
void enableLocks();
}

class QpError;
class QpSqlQuery;

class QpLockData;
class QpLock : public QObject
{
public:
    enum Status {
        UnkownStatus,
        LockedRemotely,
        LockedLocally,
        DatabaseError,
        NotLocked
    };

    static bool isLocksEnabled();
    static QpLock tryLock(QSharedPointer<QObject> object);

    QpLock();
    QpLock(const QpLock &);
    QpLock &operator=(const QpLock &);
    ~QpLock();

    Status status() const;
    QpError error() const;
    QSharedPointer<QObject> object() const;

private:    
    static void enableLocks();
    static QpLock insertLock(QSharedPointer<QObject> object);
    static QpLock selectLock(int id, QSharedPointer<QObject> object);

    friend class QpDatabaseSchema;
    friend void Qp::enableLocks();

    explicit QpLock(const QpError &error);

    int id() const;

    QSharedDataPointer<QpLockData> data;
};

#endif // LOCK_H
