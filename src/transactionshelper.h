#ifndef QPERSISTENCE_TRANSACTIONSHELPER_H
#define QPERSISTENCE_TRANSACTIONSHELPER_H

#include "defines.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QExplicitlySharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpError;
class QpStorage;
class QSqlDatabase;

class QpTransactionsHelperData;
class QpTransactionsHelper
{
public:
    static QpTransactionsHelper forStorage(const QpStorage *storage);
    QpTransactionsHelper();
    QpTransactionsHelper(const QpTransactionsHelper &other);
    QpTransactionsHelper &operator =(const QpTransactionsHelper &other);
    ~QpTransactionsHelper();

    bool begin();
    bool commitOrRollback();
    bool rollback();

    QpError lastError() const;

private:
    QExplicitlySharedDataPointer<QpTransactionsHelperData> data;

    explicit QpTransactionsHelper(const QpStorage *storage);
};

#endif // QPERSISTENCE_TRANSACTIONSHELPER_H
