#ifndef TST_SYNCHRONIZETEST_H
#define TST_SYNCHRONIZETEST_H

#include <QObject>
#include <QtTest>
#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"

class SynchronizeTest : public QObject
{
    Q_OBJECT
public:
    enum ChangerMode {
        Counter,
        OneToOne,
        OneToMany
    };

    explicit SynchronizeTest(QObject *parent = 0);

    static QList<int> childInts()
    {
        return QList<int>() << 1 << 2 << 3;
    }

    static void cleanup(QProcess *process);

private slots:
    void initDatabase();

    void testUpdateTimeChangesFromOtherProcess();
    void testUnchangedSynchronizeResult();
    void testSynchronizeCounter();
    void testSynchronizeOneToOneRelation();
    void testSynchronizeOneToManyRelation();

    void startProcess();
private:
    QProcess *startChangerProcess(int id, ChangerMode mode);
    static QProcess *m_currentProcess;
};

#endif // TST_SYNCHRONIZETEST_H
