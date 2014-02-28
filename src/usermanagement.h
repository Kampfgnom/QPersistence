#ifndef USERMANAGEMENT_H
#define USERMANAGEMENT_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QFlags>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpUserManagement
{
    QpUserManagement();
#ifndef QP_NO_USERMANAGEMENT
public:
    enum Permission {
        Select = 0x1,
        Insert = 0x2,
        Update = 0x4,
        Delete = 0x8,
        AllPermissions = Select | Insert | Update | Delete,
        NoPermission = 0x0
    };
    Q_DECLARE_FLAGS(Permissions, Permission)

    static bool createUser(const QString &username, const QString &password);
    static bool deleteUser(const QString &username);
    static bool setPassword(const QString &username, const QString &password);

    static bool grandTable(const QString &table, const QString &username, Permissions permissions);
    static bool grandTableColumn(const QString &table, const QString &column, const QString &username, Permissions permissions);
    static bool grandAll(const QString &username);
    static bool revokeAll(const QString &username);

private:
    static bool exec(const QString &query);
#endif
};

#ifndef QP_NO_USERMANAGEMENT
Q_DECLARE_OPERATORS_FOR_FLAGS(QpUserManagement::Permissions)
#endif

#endif // USERMANAGEMENT_H
