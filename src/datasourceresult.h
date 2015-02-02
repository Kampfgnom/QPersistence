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

typedef QHash<int, QpDataTransferObject> QpDataTransferObjectsById;

class QpDatasourceResultData;
class QpDatasourceResult : public QObject
{
    Q_OBJECT
public:

    explicit QpDatasourceResult(const QpDataAccessObjectBase *dao = 0);
    ~QpDatasourceResult();

    int integerResult() const;
    int size() const;
    QList<QpDataTransferObject> dataTransferObjects() const;
    QpDataTransferObjectsById dataTransferObjectsById() const;

    bool isValid() const;
    QpError lastError() const;

public slots:
    void finish();
    void reset();
    void setIntegerResult(int result);
    void setDataTransferObjects(const QpDataTransferObjectsById &dataTransferObjects);
    void addDataTransferObject(const QpDataTransferObject &dataTransferObject);
    void setLastError(const QpError &lastError);

signals:
    void finished();
    void error(const QpError &error);

private:
    QExplicitlySharedDataPointer<QpDatasourceResultData> data;
};

Q_DECLARE_METATYPE(QpDataTransferObject)
Q_DECLARE_METATYPE(QpDataTransferObjectsById)

#endif // QPERSISTENCE_DATASOURCERESULT_H
