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

class QpDatasourceResultPrivate;
class QpDatasourceResult : public QObject
{
    Q_OBJECT
public:
    explicit QpDatasourceResult(QpDataAccessObjectBase *dao = 0);
    ~QpDatasourceResult();

    struct RecordField {
        int propertyIndex;
        QString name;
        QVariant value;
    };

    struct Record {
        int primaryKey;
        QMetaObject metaObject;
        QList<RecordField> fields;
    };

    void reset();

    bool isValid() const;
    void invalidate();
    void setValid(bool valid);

    int integerResult() const;
    void setIntegerResult(int result);

    QpError error() const;
    void setError(const QpError &error);

    int size() const;
    QList<Record> records() const;
    void setRecords(const QList<Record> &records);
    void addRecord(const Record &record);

    void writeObjectProperties(QObject *object, const Record &record) const;

private:
    QExplicitlySharedDataPointer<QpDatasourceResultPrivate> data;

};

#endif // QPERSISTENCE_DATASOURCERESULT_H
