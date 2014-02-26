#ifndef TST_FLAGSTEST_H
#define TST_FLAGSTEST_H

#include "relationtestbase.h"

class FlagsTest : public QObject
{
    Q_OBJECT
public:
    explicit FlagsTest(QObject *parent = 0);

private slots:
    void initTestCase();

    void testInitialValue();
    void testReadUnknownValue();
    void testUpdate();
    void testCombined();
    void testRead();
    void testCustomCombined();
};
#endif // TST_FLAGSTEST_H
