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
};

QpError::QpError(const QString &text,
             ErrorType type,
             QVariantMap additionalInformation) :
    d(new QpErrorPrivate)
{
    d->text = text;
    d->type = type;
    d->additionalInformation = additionalInformation;
    d->isValid = (type != NoError && ! text.isEmpty());
}

QpError::~QpError()
{
}

QpError::QpError(const QpError &other)
{
    d = other.d;
}

QpError &QpError::operator =(const QpError &other)
{
    if (&other != this)
        d = other.d;

    return *this;
}

bool QpError::isValid() const
{
    return d->isValid;
}

QString QpError::text() const
{
    return d->text;
}

QpError::ErrorType QpError::type() const
{
    return d->type;
}

QVariantMap QpError::additionalInformation() const
{
    return d->additionalInformation;
}

void QpError::addAdditionalInformation(const QString &key, const QVariant &value)
{
    d->additionalInformation.insert(key, value);
}



QDebug operator<<(QDebug dbg, const QpError &error)
{
    dbg.nospace() << "(" << error.type() << ", " << error.text() << ")";
    return dbg.space();
}
