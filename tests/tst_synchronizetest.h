#ifndef TST_SYNCHRONIZETEST_H
#define TST_SYNCHRONIZETEST_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QtTest>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/condition.h"

class SynchronizeTest : public QObject
{
    Q_OBJECT
public:
    enum ChangerMode {
        Counter,
        OneToOne,
        OneToMany,
        ManyToMany,
        ChangeOnce,
        LockAndUnlock,
        LockedCounting,
        CreateAndUpdate
    };

    explicit SynchronizeTest(QObject *parent = 0);

#ifndef QP_FOR_SQLITE
    static QList<int> childInts()
    {
        return QList<int>() << 1 << 2 << 3;
    }

    static void cleanup(QProcess *process);

    void testSynchronizeCounter();

private slots:
    void init();
    void testCreatedSince();
    void testUpdatedSince();

    void testUpdateTimeChangesFromOtherProcess();
    void testUnchangedSynchronizeResult();
    void testSynchronizeOneToOneRelation();
    void testSynchronizeOneToManyRelation();
    void testSynchronizeManyToManyRelation();

    void testUpdateConflict();
    void testSynchronizeToSolveConflict();

    void startProcess();

private:
    QProcess *startChangerProcess(int id, ChangerMode mode);
    static QProcess *m_currentProcess;
#endif
};

#endif // TST_SYNCHRONIZETEST_H
