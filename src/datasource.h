#ifndef QPERSISTENCE_DATASOURCE_H
#define QPERSISTENCE_DATASOURCE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QObject;
class QpCondition;
class QpDatasourceResult;
class QpMetaObject;
class QpStorage;

class QpDatasource : public QObject
{
    Q_OBJECT

public:
    enum Order {
        Ascending,
        Descending
    };

    struct OrderField {
        QString field;
        Order order;
    };

    QpDatasource(QObject *storage = 0);
    ~QpDatasource();

    virtual void count(QpDatasourceResult *result, const QpMetaObject &metaObject, const QpCondition &condition) const = 0;
    virtual void latestRevision(QpDatasourceResult *result, const QpMetaObject &metaObject) const = 0;
    virtual void maxPrimaryKey(QpDatasourceResult *result, const QpMetaObject &metaObject) const = 0;
    virtual void objectByPrimaryKey(QpDatasourceResult *result, const QpMetaObject &metaObject, int primaryKey) const = 0;
    virtual void objects(QpDatasourceResult *result, const QpMetaObject &metaObject, int skip, int limit, const QpCondition &condition, QList<QpDatasource::OrderField> orders) const = 0;
    virtual void objectsUpdatedAfterRevision(QpDatasourceResult *result, const QpMetaObject &metaObject, int revision) const = 0;
    virtual void objectRevision(QpDatasourceResult *result, const QObject *object) const = 0;
    virtual void insertObject(QpDatasourceResult *result, const QObject *object) const = 0;
    virtual void updateObject(QpDatasourceResult *result, const QObject *object) const = 0;
    virtual void removeObject(QpDatasourceResult *result, const QObject *v) const = 0;
    virtual void incrementNumericColumn(QpDatasourceResult *result, const QObject *object, const QString &fieldName) const = 0;
};

#endif // QPERSISTENCE_DATASOURCE_H
