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
    void testGrandAll();
    void testGrandTable();
    void testSetPassword();
    void testGrandTableColumn();
    void testRevokeAll();
    void testDeleteUser();

private:
    QpUserManagement m_userManagement;
#endif
};

#endif // TST_USERMANAGEMENTTEST_H
