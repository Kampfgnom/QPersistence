#ifndef TST_CREATIONANDUPDATETIMESTEST_H
#define TST_CREATIONANDUPDATETIMESTEST_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QtTest>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class CreationAndUpdateTimesTest : public QObject
{
    Q_OBJECT
public:
    explicit CreationAndUpdateTimesTest(QObject *parent = 0);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testCreationTime();
    void testUpdateTime();

private:
    void VERIFY_QP_ERROR();
};

#endif // TST_CREATIONANDUPDATETIMESTEST_H
