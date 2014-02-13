#ifndef TST_LOCKTEST_H
#define TST_LOCKTEST_H

#include <QObject>
#include <QtTest>
#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"

class LockTest : public QObject
{
    Q_OBJECT
public:
    explicit LockTest(QObject *parent = 0);

private slots:
    void initTestCase();

    void testLockLocally();

};

#endif // TST_LOCKTEST_H
