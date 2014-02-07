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
    explicit SynchronizeTest(QObject *parent = 0);

private slots:
    void initDatabase();

    void testUpdateTimeChangesFromOtherProcess();
    void testUnchangedSynchronizeResult();
    void testSynchronizeCounter();

private:
    QProcess *changerProcess(int id);
};

#endif // TST_SYNCHRONIZETEST_H
