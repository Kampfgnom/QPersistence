#ifndef QPERSISTENCE_SQLBACKEND_H
#define QPERSISTENCE_SQLBACKEND_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QObject>
#include <QtCore/QVariant>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QSqlDatabase;

class QpSqlBackend
{
public:
    static QpSqlBackend *forDatabase(const QSqlDatabase &database);

    QpSqlBackend();
    virtual ~QpSqlBackend();
    virtual QString primaryKeyType() const = 0;
    virtual QString uniqueKeyType() const = 0;
    virtual QString variantTypeToSqlType(QVariant::Type type) const = 0;
    virtual QString nowTimestamp() const = 0;
    virtual QString orIgnore() const = 0;
};

class QpSqliteBackend : public QpSqlBackend
{
public:
    QpSqliteBackend();
    QString primaryKeyType() const Q_DECL_OVERRIDE;
    QString uniqueKeyType() const Q_DECL_OVERRIDE;
    QString variantTypeToSqlType(QVariant::Type type) const Q_DECL_OVERRIDE;
    QString nowTimestamp() const Q_DECL_OVERRIDE;
    QString orIgnore() const Q_DECL_OVERRIDE;
};

class QpMySqlBackend : public QpSqlBackend
{
public:
    QpMySqlBackend();
    QString primaryKeyType() const Q_DECL_OVERRIDE;
    QString uniqueKeyType() const Q_DECL_OVERRIDE;
    QString variantTypeToSqlType(QVariant::Type type) const Q_DECL_OVERRIDE;
    QString nowTimestamp() const Q_DECL_OVERRIDE;
    QString orIgnore() const Q_DECL_OVERRIDE;
};

#endif // QPERSISTENCE_SQLBACKEND_H
