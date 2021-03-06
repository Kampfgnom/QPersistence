#include "usermanagement.h"

#include "storage.h"

#ifdef QP_NO_USERMANAGEMENT
QpUserManagement::QpUserManagement()
{
}
#endif

#ifndef QP_NO_USERMANAGEMENT

#include "error.h"
#include "qpersistence.h"
#include "sqlquery.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

QpUserManagement::QpUserManagement(QpStorage *s) :
    storage(s)
{
}

bool QpUserManagement::createUser(const QString &username, const QString &password)
{
    return exec(QString("CREATE USER '%1' IDENTIFIED BY '%2';"
                        "FLUSH PRIVILEGES;")
                .arg(username)
                .arg(password));
}

bool QpUserManagement::deleteUser(const QString &username)
{
    return exec(QString("DROP USER '%1'@'%'; "
                        "FLUSH PRIVILEGES;")
                .arg(username));
}

bool QpUserManagement::setPassword(const QString &username, const QString &password)
{

    return exec(QString("SET PASSWORD FOR '%1'@'%' = PASSWORD('%2');"
                        "FLUSH PRIVILEGES;")
                .arg(username)
                .arg(password));
}

bool QpUserManagement::grandTable(const QString &table, const QString &username, QpUserManagement::Permissions permissions)
{
    QStringList l;
    if (permissions & Select) l.append("SELECT");
    if (permissions & Insert) l.append("INSERT");
    if (permissions & Update) l.append("UPDATE");
    if (permissions & Delete) l.append("DELETE");

    return exec("GRANT " + l.join(",") + " "
                "ON `" + table + "` "
                "TO '" + username + "'@'%'; "
                "FLUSH PRIVILEGES;");
}

bool QpUserManagement::grandTableColumn(const QString &table, const QString &column, const QString &username, QpUserManagement::Permissions permissions)
{
    QStringList l;

    if (permissions & Select) l.append("SELECT");
    if (permissions & Update) l.append("UPDATE");

    return exec("GRANT " + l.join(",") + " (`" + column + "`) "
                "ON `" + table + "` "
                "TO '" + username + "'@'%'; "
                "FLUSH PRIVILEGES;");
}

bool QpUserManagement::grandAll(const QString &username, bool withGrantOption)
{
    return exec(QString(
                        "GRANT RELOAD, CREATE USER "
                        "ON *.* "
                        "TO '" + username + "'@'%' "
                        "WITH GRANT OPTION; "
                        "GRANT SELECT, INSERT, UPDATE, DELETE "
                        "ON `" + storage->database().databaseName() + "`.* "
                        "TO '" + username + "'@'%'; "
                        "GRANT SELECT, INSERT, UPDATE, DELETE "
                        "ON `mysql`.* "
                        "TO '" + username + "'@'%' "
                        "%1"
                        "FLUSH PRIVILEGES;")
                .arg(withGrantOption ? "WITH GRANT OPTION; " : ""));
}

bool QpUserManagement::revokeAll(const QString &username)
{
    return exec("REVOKE RELOAD, CREATE USER "
                "ON *.* "
                "FROM '" + username + "'@'%'; "
                "REVOKE ALL PRIVILEGES "
                "ON `" + storage->database().databaseName() + "`.*, mysql.* "
                "FROM '" + username + "'@'%'; "
                "REVOKE GRANT OPTION ON *.* FROM '" + username + "'@'%'; "
                "FLUSH PRIVILEGES;");
}

bool QpUserManagement::exec(const QString &query)
{
    QpSqlQuery q(storage->database());
    if (!q.exec(query)) {
        storage->setLastError(q);
        return false;
    }
    return true;
}
#endif
