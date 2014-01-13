#ifndef QPERSISTENCE_SQLBACKEND_H
#define QPERSISTENCE_SQLBACKEND_H

#include <QExplicitlySharedDataPointer>

class QSqlDatabase;

class QpSqlBackendData;
class QpSqlBackend
{
public:
    static QpSqlBackend forDatabase(const QSqlDatabase &database);

    QpSqlBackend();
    QpSqlBackend(const QpSqlBackend &);
    QpSqlBackend &operator=(const QpSqlBackend &);
    ~QpSqlBackend();

    QString primaryKeyType() const;

protected:
    void setPrimaryKeyType(const QString &type);

private:
    QExplicitlySharedDataPointer<QpSqlBackendData> data;
};

class QpSqliteBackend : public QpSqlBackend
{
public:
    QpSqliteBackend();
};

class QpMySqlBackend : public QpSqlBackend
{
public:
    QpMySqlBackend();
};

#endif // QPERSISTENCE_SQLBACKEND_H
