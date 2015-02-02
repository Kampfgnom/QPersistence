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
        m_userManagement = QpUserManagement(QpStorage::defaultStorage());
    }
}

void UserManagementTest::testCreateUser()
{
    QVERIFY(m_userManagement.createUser("testUser", "password"));
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("SELECT `User` FROM `mysql`.`user` WHERE `User` = 'testUser'"));
    QVERIFY(query.first());
    QCOMPARE(query.value(0).toString(), QString("testUser"));
}

void UserManagementTest::testSetPassword()
{
    QVERIFY(m_userManagement.setPassword("testUser", "asdasd"));
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "testconnection");
    db.setHostName(QpStorage::defaultStorage()->database().hostName());
    db.setDatabaseName(QpStorage::defaultStorage()->database().databaseName());
    db.setUserName("testUser");
    db.setPassword("asdasd");
    if(!db.open()) {
        QFAIL(db.lastError().text().toLatin1());
    }
    db.close();
    QSqlDatabase::removeDatabase("testconnection");
}

void UserManagementTest::testGrandTable()
{
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("CREATE TABLE `testTable` (id INTEGER PRIMARY KEY)"));
    QVERIFY(m_userManagement.grandTable("testTable", "testUser", QpUserManagement::Insert));

    // TODO: Verify correct permissions of QpUserManagement::grandTable
    // How would I test that?
}

void UserManagementTest::testGrandTableColumn()
{
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("CREATE TABLE `testTable2` (`id` INTEGER PRIMARY KEY, `column` TEXT)"));
    QVERIFY(m_userManagement.grandTableColumn("testTable2", "column", "testUser", QpUserManagement::Update | QpUserManagement::Insert));
    QVERIFY(query.exec("DROP TABLE `testTable2`"));

    // TODO: Verify correct permissions of QpUserManagement::grandTableColumn
    // How would I test that?
}

void UserManagementTest::testGrandAll()
{
    QVERIFY(m_userManagement.grandAll("testUser"));

    // TODO: Verify correct permissions of QpUserManagement::grandAll
    // How would I test that?
}

void UserManagementTest::testRevokeAll()
{
    QVERIFY(m_userManagement.revokeAll("testUser"));

    // TODO: Verify correct permissions of QpUserManagement::v
    // How would I test that?
}

void UserManagementTest::testDeleteUser()
{
    QVERIFY(m_userManagement.deleteUser("testUser"));
    QSqlQuery query(Qp::database());
    QVERIFY(query.exec("SELECT `User` FROM `mysql`.`user` WHERE `User` = 'testUser';"));
    QCOMPARE(query.size(), 0);
}
#endif
