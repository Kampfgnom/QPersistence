#include "error.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QCoreApplication>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS


/******************************************************************************
 * QpErrorData
 */
class QpErrorData : public QSharedData
{
public:
    QpErrorData() :
        QSharedData(),
        text(QString()),
        type(QpError::NoError),
        isValid(false)
    {
    }

    static QpError *lastError;

    QSqlError sqlError;
    QString text;
    QVariantMap additionalInformation;
    QpError::ErrorType type;
    bool isValid;
    QString lastQuery;
    QMap<QString, QVariant> boundValues;
};


/******************************************************************************
 * QpError
 */
QpError::QpError(const QString &text,
                 ErrorType type,
                 QVariantMap additionalInformation) :
    data(new QpErrorData)
{
    data->text = text;
    data->type = type;
    data->additionalInformation = additionalInformation;
    data->isValid = (type != NoError && !text.isEmpty());
}

QpError::QpError(const QSqlQuery &query) :
    QpError(query.lastError())
{
    data->lastQuery = query.lastQuery();
    data->boundValues = query.boundValues();
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

QString QpError::lastQuery() const
{
    return data->lastQuery;
}

QMap<QString, QVariant> QpError::boundValues() const
{
    return data->boundValues;
}

QVariantMap QpError::additionalInformation() const
{
    return data->additionalInformation;
}

void QpError::addAdditionalInformation(const QString &key, const QVariant &value)
{
    data->additionalInformation.insert(key, value);
}

QDebug operator<<(QDebug dbg, const QpError &error)
{
    dbg.nospace() << "(" << error.type() << ", " << error.text() << ")\n" << error.lastQuery();
    return dbg.space();
}


QpAbstractErrorHandler::QpAbstractErrorHandler(QObject *parent) :
    QObject(parent)
{
}

QpAbstractErrorHandler::~QpAbstractErrorHandler()
{
}

QpPrintError::QpPrintError(QObject *parent) :
    QpAbstractErrorHandler(parent)
{
}

QpPrintError::~QpPrintError()
{
}

void QpPrintError::handleError(const QpError &error)
{
    qWarning() << qPrintable(error.text());
}


QpQuitOnError::QpQuitOnError(QObject *parent) :
    QpAbstractErrorHandler(parent)
{
}

QpQuitOnError::~QpQuitOnError()
{
}

void QpQuitOnError::handleError(const QpError &error)
{
    Q_UNUSED(error);
    Q_ASSERT(false);
    qApp->exit(-1);
}
