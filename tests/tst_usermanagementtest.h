#ifndef TST_USERMANAGEMENTTEST_H
#define TST_USERMANAGEMENTTEST_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QtTest>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include <QPersistence.h>
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"

class UserManagementTest : public QObject
{
    Q_OBJECT
public:
    explicit UserManagementTest(QObject *parent = 0);

private slots:
    void initDatabase();

    void testCreateUser();
    void testSetPassword();
    void testGrandTable();
    void testGrandTableColumn();
    void testGrandAll();
    void testRevokeAll();
    void testDeleteUser();
};

#endif // TST_USERMANAGEMENTTEST_H
