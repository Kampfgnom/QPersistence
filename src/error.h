#ifndef QPERSISTENCE_ERROR_H
#define QPERSISTENCE_ERROR_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpError;

namespace Qp {
namespace Private {
void setLastError(const QpError &);
}
}

class QSqlError;
class QSqlQuery;

class QpErrorData;
class QpError
{
public:
    enum ErrorType {
        NoError = 0,
        SqlError,
        TransactionError,
        UserError = 1024
    };

    static QpError lastError();

    QpError(const QString &text = QString(),
            ErrorType type = NoError,
            QVariantMap additionalInformation = QVariantMap());
    QpError(const QSqlError &error);
    ~QpError();
    QpError(const QpError &other);
    QpError &operator = (const QpError &other);

    bool isValid() const;
    QString text() const;
    ErrorType type() const;

    QVariantMap additionalInformation() const;
    void addAdditionalInformation(const QString &key, const QVariant &value);

private:
    friend void Qp::Private::setLastError(const QpError &);
    static void setLastError(const QpError error);

    QSharedDataPointer<QpErrorData> data;
};

QDebug operator<<(QDebug dbg, const QpError &error);

#endif // QPERSISTENCE_ERROR_H
