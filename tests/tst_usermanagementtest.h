#ifndef TST_USERMANAGEMENTTEST_H
#define TST_USERMANAGEMENTTEST_H

#include "tests_common.h"

class UserManagementTest : public QObject
{
    Q_OBJECT
#ifndef QP_NO_USERMANAGEMENT
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
#endif
};

#endif // TST_USERMANAGEMENTTEST_H
