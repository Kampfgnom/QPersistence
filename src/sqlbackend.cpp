#include "sqlbackend.h"
#include <QSharedData>

#include <QHash>
#include <QSqlDatabase>


class QpSqlBackendData : public QSharedData
{
public:
    QpSqlBackendData() : QSharedData()
    {}

    QString primaryKeyType;
};

QpSqlBackend QpSqlBackend::forDatabase(const QSqlDatabase &database)
{
    if(database.driverName() == QLatin1String("QSQLITE"))
        return QpSqliteBackend();
    if(database.driverName() == QLatin1String("QMYSQL"))
        return QpMySqlBackend();


    Q_ASSERT_X(false, Q_FUNC_INFO,
               QString("No backend for database '%1'")
               .arg(database.driverName())
               .toLatin1());

    return QpSqlBackend(); // Never reached
}

QpSqlBackend::QpSqlBackend() :
    data(new QpSqlBackendData)
{
}

QpSqlBackend::QpSqlBackend(const QpSqlBackend &rhs) :
    data(rhs.data)
{
}

QpSqlBackend &QpSqlBackend::operator=(const QpSqlBackend &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);

    return *this;
}

QpSqlBackend::~QpSqlBackend()
{
}

QString QpSqlBackend::primaryKeyType() const
{
    Q_ASSERT(!data->primaryKeyType.isEmpty());
    return data->primaryKeyType;
}

void QpSqlBackend::setPrimaryKeyType(const QString &type)
{
    data->primaryKeyType = type;
}

QpSqliteBackend::QpSqliteBackend()
{
    setPrimaryKeyType(QLatin1String("INTEGER PRIMARY KEY AUTOINCREMENT"));
}


QpMySqlBackend::QpMySqlBackend()
{
    setPrimaryKeyType(QLatin1String("INTEGER PRIMARY KEY AUTO_INCREMENT"));
}
