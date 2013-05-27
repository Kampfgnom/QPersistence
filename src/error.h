#ifndef QPERSISTENCE_ERROR_H
#define QPERSISTENCE_ERROR_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

class QSqlQuery;

class QpErrorPrivate;
class QpError
{
public:
    enum ErrorType {
        NoError = 0,
        SqlError,
        ParserError,
        SerializerError,
        ServerError,
        StorageError,
        UserError = 1024
    };

    QpError(const QString &text = QString(),
          ErrorType type = NoError,
          QVariantMap additionalInformation = QVariantMap());
    ~QpError();
    QpError(const QpError &other);
    QpError &operator = (const QpError &other);

    bool isValid() const;
    QString text() const;
    ErrorType type() const;

    QVariantMap additionalInformation() const;
    void addAdditionalInformation(const QString &key, const QVariant &value);

private:
    QSharedDataPointer<QpErrorPrivate> d;
};

QDebug operator<<(QDebug dbg, const QpError &error);

#endif // QPERSISTENCE_ERROR_H
