#ifndef TST_ENUMERATIONTEST_H
#define TST_ENUMERATIONTEST_H

#include "tests_common.h"

class EnumerationTest : public QObject
{
    Q_OBJECT
public:
    explicit EnumerationTest(QObject *parent = 0);

private slots:
    void testInitialEnumValue();
    void testUpdateEnum();
    void testExplicitValue();
    void testReadEnum();
};

#endif // TST_ENUMERATIONTEST_H
