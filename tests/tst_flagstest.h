#ifndef TST_FLAGSTEST_H
#define TST_FLAGSTEST_H

#include "tests_common.h"

class FlagsTest : public QObject
{
    Q_OBJECT
public:
    explicit FlagsTest(QObject *parent = 0);

private slots:
    void testInitialValue();
    void testReadUnknownValue();
    void testUpdate();
    void testCombined();
    void testRead();
    void testCustomCombined();
};
#endif // TST_FLAGSTEST_H
