#ifndef QPERSISTENCE_ERROR_H
#define QPERSISTENCE_ERROR_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

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
        TransactionRequestedByApplication,
        UpdateConflictError,
        UserError = 1024
    };

    QpError(const QString &text = QString(),
            ErrorType type = NoError,
            QVariantMap additionalInformation = QVariantMap());
    QpError(const QSqlQuery &query);
    QpError(const QSqlError &error);
    ~QpError();
    QpError(const QpError &other);
    QpError &operator = (const QpError &other);

    bool isValid() const;
    QString text() const;
    ErrorType type() const;
    QString lastQuery() const;
    QMap<QString, QVariant> boundValues() const;

    QVariantMap additionalInformation() const;
    void addAdditionalInformation(const QString &key, const QVariant &value);

private:
    QSharedDataPointer<QpErrorData> data;
};

class QpAbstractErrorHandler : public QObject
{
    Q_OBJECT
public:
    explicit QpAbstractErrorHandler(QObject *parent = 0);
    ~QpAbstractErrorHandler();
    virtual void handleError(const QpError &error) = 0;
};

class QpPrintError : public QpAbstractErrorHandler
{
    Q_OBJECT
public:
    explicit QpPrintError(QObject *parent = 0);
    ~QpPrintError();
    void handleError(const QpError &error);
};

class QpQuitOnError : public QpAbstractErrorHandler
{
    Q_OBJECT
public:
    explicit QpQuitOnError(QObject *parent = 0);
    ~QpQuitOnError();
    void handleError(const QpError &error);
};

QDebug operator<<(QDebug dbg, const QpError &error);

Q_DECLARE_METATYPE(QpError)

#endif // QPERSISTENCE_ERROR_H
