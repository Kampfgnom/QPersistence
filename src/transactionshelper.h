#ifndef QPERSISTENCE_TRANSACTIONSHELPER_H
#define QPERSISTENCE_TRANSACTIONSHELPER_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QExplicitlySharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpStorage;
class QSqlDatabase;

class QpTransactionsHelperData;
class QpTransactionsHelper
{
public:
    explicit QpTransactionsHelper(QpStorage *storage);
    QpTransactionsHelper(const QpTransactionsHelper &other);
    QpTransactionsHelper &operator =(const QpTransactionsHelper &other);
    ~QpTransactionsHelper();

    bool begin();
    bool commitOrRollback();
    bool rollback();

private:
    QExplicitlySharedDataPointer<QpTransactionsHelperData> data;

};

#endif // QPERSISTENCE_TRANSACTIONSHELPER_H
