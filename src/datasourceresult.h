#ifndef QPERSISTENCE_DATASOURCERESULT_H
#define QPERSISTENCE_DATASOURCERESULT_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QMetaProperty>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpError;
class QpDataAccessObjectBase;

class QpDataTransferObject
{
public:
    QpDataTransferObject();
    int primaryKey;
    QMetaObject metaObject;
    QHash<int, QVariant> properties;
    QHash<QString, QVariant> dynamicProperties;
    QHash<int, int> toOneRelationFKs;
    QHash<int, QList<int>> toManyRelationFKs;

    void write(QObject *object) const;
    static QpDataTransferObject fromObject(const QObject *object); //! The "_Qp_dataTransferObject" dynamic property
};

class QpDatasourceResultPrivate;
class QpDatasourceResult : public QObject
{
    Q_OBJECT
public:
    explicit QpDatasourceResult(QpDataAccessObjectBase *dao = 0);
    ~QpDatasourceResult();

    void reset();

    bool isValid() const;
    void invalidate();
    void setValid(bool valid);

    int integerResult() const;
    void setIntegerResult(int result);

    QpError error() const;
    void setError(const QpError &error);

    int size() const;
    QList<QpDataTransferObject> dataTransferObjects() const;
    QHash<int, QpDataTransferObject> dataTransferObjectsById() const;
    void setDataTransferObjects(const QHash<int, QpDataTransferObject> &dataTransferObjects);
    void addDataTransferObject(const QpDataTransferObject &dataTransferObject);

private:
    QExplicitlySharedDataPointer<QpDatasourceResultPrivate> data;

};

Q_DECLARE_METATYPE(QpDataTransferObject)

#endif // QPERSISTENCE_DATASOURCERESULT_H
