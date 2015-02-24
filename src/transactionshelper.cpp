#include "transactionshelper.h"

#include "error.h"
#include "storage.h"

#include <QSqlDatabase>
#include <QSqlError>

/******************************************************************************
 * QpTransactionsData
 */
class QpTransactionsHelperData : public QSharedData
{
public:
    QpTransactionsHelperData() :
        QSharedData(),
        storage(nullptr),
        transactionLevel(0)
    {
    }

    QpStorage *storage;
    int transactionLevel;

    bool transactionRecursive();
    bool commitRecursive();
    bool rollback();
};

bool QpTransactionsHelperData::transactionRecursive()
{
    Q_ASSERT(storage->database().isOpen());

    ++transactionLevel;
    if (transactionLevel > 1)
        return true;

    Q_ASSERT(transactionLevel == 1);

    if (!storage->database().transaction()) {
        storage->setLastError(storage->database().lastError());
        return false;
    }

    return true;
}

bool QpTransactionsHelperData::commitRecursive()
{
    Q_ASSERT(storage->database().isOpen());
    Q_ASSERT(transactionLevel > 0);

    --transactionLevel;
    if (transactionLevel > 0)
        return true;

    Q_ASSERT(transactionLevel == 0);

    if (!storage->database().commit()) {
        storage->setLastError(storage->database().lastError());
        return false;
    }

    return true;
}

bool QpTransactionsHelperData::rollback()
{
    Q_ASSERT(storage->database().isOpen());
    Q_ASSERT(transactionLevel > 0);

    --transactionLevel;
    if (transactionLevel > 0)
        return true;

    Q_ASSERT(transactionLevel == 0);

    if (!storage->database().rollback()) {
        storage->setLastError(storage->database().lastError());
        return false;
    }

    return true;
}

/******************************************************************************
 * QpTransactions
 */
QpTransactionsHelper::QpTransactionsHelper(QpStorage *storage) :
    data(new QpTransactionsHelperData)
{
    data->storage = storage;
}

QpTransactionsHelper::QpTransactionsHelper(const QpTransactionsHelper &other) :
    data(other.data)
{
}

QpTransactionsHelper::~QpTransactionsHelper()
{
}

QpTransactionsHelper &QpTransactionsHelper::operator =(const QpTransactionsHelper &other)
{
    if (this != &other)
        data = other.data;
    return *this;
}

bool QpTransactionsHelper::begin()
{
    return data->transactionRecursive();
}

bool QpTransactionsHelper::commitOrRollback()
{
    if (!data->storage->lastError().isValid())
        return data->commitRecursive();

    data->rollback();
    return false;
}

bool QpTransactionsHelper::rollback()
{
    data->storage->setLastError(QpError("Application code requested rollback", QpError::TransactionRequestedByApplication));
    return data->rollback();
}
