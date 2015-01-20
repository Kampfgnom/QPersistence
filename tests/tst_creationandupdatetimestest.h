#ifndef TST_CREATIONANDUPDATETIMESTEST_H
#define TST_CREATIONANDUPDATETIMESTEST_H

#include "tests_common.h"

class CreationAndUpdateTimesTest : public QObject
{
    Q_OBJECT
public:
    explicit CreationAndUpdateTimesTest(QObject *parent = 0);

#ifndef QP_NO_TIMESTAMPS
private Q_SLOTS:
    void init();
    void cleanupTestCase();

    void testCreationTime();
    void testUpdateTime();

private:
    void VERIFY_QP_ERROR();
#endif
};

#endif // TST_CREATIONANDUPDATETIMESTEST_H
