#ifndef QPERSISTENCE_LEGACYSQLDATASOURCE_H
#define QPERSISTENCE_LEGACYSQLDATASOURCE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QSharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "datasource.h"

class QSqlDatabase;
class QpStorage;
class QpSqlQuery;

class QpLegacySqlDatasourceData;
class QpLegacySqlDatasource : public QpDatasource
{
public:
    QpLegacySqlDatasource(QObject *parent = 0);
    ~QpLegacySqlDatasource();

    QSqlDatabase database() const;
    void setSqlDatabase(const QSqlDatabase &database);

    void count(QpDatasourceResult *result, const QpMetaObject &metaObject, const QpCondition &condition) const Q_DECL_OVERRIDE;
    void latestRevision(QpDatasourceResult *result, const QpMetaObject &metaObject) const Q_DECL_OVERRIDE;
    void maxPrimaryKey(QpDatasourceResult *result, const QpMetaObject &metaObject) const Q_DECL_OVERRIDE;
    void objectByPrimaryKey(QpDatasourceResult *result, const QpMetaObject &metaObject, int primaryKey) const Q_DECL_OVERRIDE;
    void objects(QpDatasourceResult *result, const QpMetaObject &metaObject, int skip, int limit, const QpCondition &condition, QList<QpDatasource::OrderField> orders) const Q_DECL_OVERRIDE;
    void objectsUpdatedAfterRevision(QpDatasourceResult *result, const QpMetaObject &metaObject, int revision) const Q_DECL_OVERRIDE;
    void objectRevision(QpDatasourceResult *result, const QObject *object) const Q_DECL_OVERRIDE;
    void insertObject(QpDatasourceResult *result, const QObject *object) const Q_DECL_OVERRIDE;
    void updateObject(QpDatasourceResult *result, const QObject *object) const Q_DECL_OVERRIDE;
    void removeObject(QpDatasourceResult *result, const QObject *object) const Q_DECL_OVERRIDE;
    void incrementNumericColumn(QpDatasourceResult *result, const QObject *object, const QString &fieldName) const Q_DECL_OVERRIDE;


private:
    QSharedDataPointer<QpLegacySqlDatasourceData> data;
};

#endif // QPERSISTENCE_LEGACYSQLDATASOURCE_H
