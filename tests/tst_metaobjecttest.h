#ifndef TST_METAOBJECTTEST_H
#define TST_METAOBJECTTEST_H

#include <QString>
#include <QtTest>

class MetaObjectTest : public QObject
{
    Q_OBJECT

public:
    MetaObjectTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
};

#endif // TST_METAOBJECTTEST_H
