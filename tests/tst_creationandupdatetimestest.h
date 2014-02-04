#ifndef TST_CREATIONANDUPDATETIMESTEST_H
#define TST_CREATIONANDUPDATETIMESTEST_H

#include <QObject>
#include <QtTest>

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
