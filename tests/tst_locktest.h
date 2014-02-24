#ifndef TST_LOCKTEST_H
#define TST_LOCKTEST_H

#include <QObject>
#include <QtTest>
#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"
#include "tst_synchronizetest.h"

class LockTest : public QObject
{
    Q_OBJECT
public:
    explicit LockTest(QObject *parent = 0);

    static void cleanup(QProcess *process);

    void testSynchronizedCounter();

    static QHash<QString, QVariant> info()
    {
        QHash<QString, QVariant> info;
        info.insert("aString", "asdagasg");
        info.insert("someNUmber", 123415);
        info.insert("date", QDate(123,12,24));
        return info;
    }

private slots:
    void startProcess();

    void initTestCase();

    void testLockAndUnlockLocally();
    void testLockAndUnlockRemotely();
    void testLockRemotelyAndUnlockLocally();

    void testLockInformationLocal();
    void testLockInformationRemote();

private:
    QProcess *startChangerProcess(int id, SynchronizeTest::ChangerMode mode);
    static QProcess *m_currentProcess;
};

#endif // TST_LOCKTEST_H
