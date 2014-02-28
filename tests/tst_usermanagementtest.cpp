#include "tst_usermanagementtest.h"

#ifndef QP_NO_USERMANAGEMENT
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

UserManagementTest::UserManagementTest(QObject *parent) :
    QObject(parent)
{
}

void UserManagementTest::initDatabase()
{
    QSqlDatabase db = Qp::database();
    Qp::setSqlDebugEnabled(true);
    if(!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("192.168.100.2");
        db.setDatabaseName("niklas");
        db.setUserName("niklas");
        db.setPassword("niklas");

        Qp::setDatabase(db);
    }
}

void UserManagementTest::testCreateUser()
{
    QVERIFY(QpUserManagement::createUser("testUser", "password"));
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("SELECT User FROM mysql.user WHERE User = testUser;"));
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toString(), QString("testUser"));
}

void UserManagementTest::testSetPassword()
{
    QVERIFY(QpUserManagement::setPassword("testUser", "asdasd"));
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "testconnection");
    db.setHostName("192.168.100.2");
    db.setDatabaseName("niklas");
    db.setUserName("testUser");
    db.setPassword("asdasd");
    QVERIFY(db.open());
    db.close();
    QSqlDatabase::removeDatabase("testconnection");
}

void UserManagementTest::testGrandTable()
{
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("CREATE TABLE testTable (id INTEGER PRIMARY KEY)"));
    QVERIFY(QpUserManagement::grandTable("testTable", "testUser", QpUserManagement::Insert));

    // TODO: Verify correct permissions of QpUserManagement::grandTable
    // How would I test that?
}

void UserManagementTest::testGrandTableColumn()
{
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("CREATE TABLE testTable2 (id INTEGER PRIMARY KEY, column TEXT)"));
    QVERIFY(QpUserManagement::grandTableColumn("testTable2", "column", "testUser", QpUserManagement::Update | QpUserManagement::Insert));
    QVERIFY(query.exec("DROP TABLE testTable2"));

    // TODO: Verify correct permissions of QpUserManagement::grandTableColumn
    // How would I test that?
}

void UserManagementTest::testGrandAll()
{
    QVERIFY(QpUserManagement::grandAll("testUser"));

    // TODO: Verify correct permissions of QpUserManagement::grandAll
    // How would I test that?
}

void UserManagementTest::testRevokeAll()
{
    QVERIFY(QpUserManagement::revokeAll("testUser"));

    // TODO: Verify correct permissions of QpUserManagement::v
    // How would I test that?
}

void UserManagementTest::testDeleteUser()
{
    QVERIFY(QpUserManagement::deleteUser("testUser"));
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("SELECT User FROM mysql.user WHERE User = testUser;"));
    QCOMPARE(query.size(), 0);
}
#endif
