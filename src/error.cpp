#include "error.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QDebug>
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpErrorData : public QSharedData
{
    public:
        QpErrorData() :
            QSharedData(),
            text(QString()),
            type(QpError::NoError),
            isValid(false)
        {}

        static QpError *lastError;

        QSqlError sqlError;
        QString text;
        QVariantMap additionalInformation;
        QpError::ErrorType type;
        bool isValid;
};

QpError *QpErrorData::lastError(nullptr);

QpError QpError::lastError()
{
    if(!QpErrorData::lastError)
        QpErrorData::lastError = new QpError;

    return *QpErrorData::lastError;
}

QpError::QpError(const QString &text,
             ErrorType type,
             QVariantMap additionalInformation) :
    data(new QpErrorData)
{
    data->text = text;
    data->type = type;
    data->additionalInformation = additionalInformation;
    data->isValid = (type != NoError && ! text.isEmpty());
}

QpError::QpError(const QSqlError &error) :
    data(new QpErrorData)
{
    data->sqlError = error;
    data->text = error.text();
    data->type = SqlError;
    data->isValid = error.isValid();
}

QpError::~QpError()
{
}

QpError::QpError(const QpError &other)
{
    data = other.data;
}

QpError &QpError::operator =(const QpError &other)
{
    if (&other != this)
        data = other.data;

    return *this;
}

bool QpError::isValid() const
{
    return data->isValid;
}

QString QpError::text() const
{
    return data->text;
}

QpError::ErrorType QpError::type() const
{
    return data->type;
}

QVariantMap QpError::additionalInformation() const
{
    return data->additionalInformation;
}

void QpError::addAdditionalInformation(const QString &key, const QVariant &value)
{
    data->additionalInformation.insert(key, value);
}

void QpError::setLastError(const QpError error)
{
    lastError();
    (*QpErrorData::lastError) = error;
}

QDebug operator<<(QDebug dbg, const QpError &error)
{
    dbg.nospace() << "(" << error.type() << ", " << error.text() << ")";
    return dbg.space();
}
