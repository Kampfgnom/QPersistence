#include "error.h"

#include <QDebug>

class QpErrorPrivate : public QSharedData
{
    public:
        QpErrorPrivate() :
            QSharedData(),
            text(QString()),
            isValid(false),
            type(QpError::NoError)
        {}

        QString text;
        bool isValid;
        QVariantMap additionalInformation;
        QpError::ErrorType type;

        static QpError lastError;
};

QpError QpErrorPrivate::lastError;

QpError QpError::lastError()
{
    return QpErrorPrivate::lastError;
}

QpError::QpError(const QString &text,
             ErrorType type,
             QVariantMap additionalInformation) :
    data(new QpErrorPrivate)
{
    data->text = text;
    data->type = type;
    data->additionalInformation = additionalInformation;
    data->isValid = (type != NoError && ! text.isEmpty());
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
    QpErrorPrivate::lastError = error;
}

QDebug operator<<(QDebug dbg, const QpError &error)
{
    dbg.nospace() << "(" << error.type() << ", " << error.text() << ")";
    return dbg.space();
}
