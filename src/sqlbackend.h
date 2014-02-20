#ifndef QPERSISTENCE_SQLBACKEND_H
#define QPERSISTENCE_SQLBACKEND_H

#include <QExplicitlySharedDataPointer>
#include <QObject>
#include <QVariant>

class QSqlDatabase;

class QpSqlBackend : public QObject
{
public:
    static QpSqlBackend *forDatabase(const QSqlDatabase &database);

    QpSqlBackend(QObject *parent);
    ~QpSqlBackend();

    virtual QString primaryKeyType() const = 0;
    virtual QString uniqueKeyType() const = 0;
    virtual QString variantTypeToSqlType(QVariant::Type type) const = 0;
    virtual QString nowTimestamp() const = 0;
    virtual QString orIgnore() const = 0;
};

class QpSqliteBackend : public QpSqlBackend
{
public:
    QpSqliteBackend(QObject *parent);
    QString primaryKeyType() const Q_DECL_OVERRIDE;
    QString uniqueKeyType() const Q_DECL_OVERRIDE;
    QString variantTypeToSqlType(QVariant::Type type) const Q_DECL_OVERRIDE;
    QString nowTimestamp() const Q_DECL_OVERRIDE;
    QString orIgnore() const Q_DECL_OVERRIDE;
};

class QpMySqlBackend : public QpSqlBackend
{
public:
    QpMySqlBackend(QObject *parent);
    QString primaryKeyType() const Q_DECL_OVERRIDE;
    QString uniqueKeyType() const Q_DECL_OVERRIDE;
    QString variantTypeToSqlType(QVariant::Type type) const Q_DECL_OVERRIDE;
    QString nowTimestamp() const Q_DECL_OVERRIDE;
    QString orIgnore() const Q_DECL_OVERRIDE;
};

#endif // QPERSISTENCE_SQLBACKEND_H
