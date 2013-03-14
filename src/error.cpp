#include "error.h"

#include <QDebug>



class QPersistenceErrorPrivate : public QSharedData
{
    public:
        QPersistenceErrorPrivate() :
            QSharedData(),
            text(QString()),
            isValid(false),
            type(QPersistenceError::NoError)
        {}

        QString text;
        bool isValid;
        QVariantMap additionalInformation;
        QPersistenceError::ErrorType type;
};

QPersistenceError::QPersistenceError(const QString &text,
             ErrorType type,
             QVariantMap additionalInformation) :
    d(new QPersistenceErrorPrivate)
{
    d->text = text;
    d->type = type;
    d->additionalInformation = additionalInformation;
    d->isValid = (type != NoError && ! text.isEmpty());
}

QPersistenceError::~QPersistenceError()
{
}

QPersistenceError::QPersistenceError(const QPersistenceError &other)
{
    d = other.d;
}

QPersistenceError &QPersistenceError::operator =(const QPersistenceError &other)
{
    if (&other != this)
        d = other.d;

    return *this;
}

bool QPersistenceError::isValid() const
{
    return d->isValid;
}

QString QPersistenceError::text() const
{
    return d->text;
}

QPersistenceError::ErrorType QPersistenceError::type() const
{
    return d->type;
}

QVariantMap QPersistenceError::additionalInformation() const
{
    return d->additionalInformation;
}

void QPersistenceError::addAdditionalInformation(const QString &key, const QVariant &value)
{
    d->additionalInformation.insert(key, value);
}



QDebug operator<<(QDebug dbg, const QPersistenceError &error)
{
    dbg.nospace() << "(" << error.type() << ", " << error.text() << ")";
    return dbg.space();
}
