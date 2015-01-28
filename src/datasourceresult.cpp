#include "datasourceresult.h"

#include "dataaccessobject.h"
#include "error.h"
#include "storage.h"

/******************************************************************************
 * QpDatasourceResultData
 */
class QpDatasourceResultPrivate : public QSharedData
{
public:
    QpDatasourceResultPrivate();

    QpDataAccessObjectBase *dataAccessObject;

    bool valid;
    int integerResult;
    QpError error;
    QList<QpDatasourceResult::Record> records;
};

QpDatasourceResultPrivate::QpDatasourceResultPrivate() :
    QSharedData(),
    dataAccessObject(nullptr),
    valid(false),
    integerResult(-1)
{
}

/******************************************************************************
 * QpDatasourceResult
 */
QpDatasourceResult::QpDatasourceResult(QpDataAccessObjectBase *dao) :
    QObject(dao),
    data(new QpDatasourceResultPrivate)
{
    data->dataAccessObject = dao;
}

QpDatasourceResult::~QpDatasourceResult()
{
}

void QpDatasourceResult::reset()
{
    data->valid = false;
    data->integerResult = -1;
    data->error = QpError();
    data->records.clear();
}

bool QpDatasourceResult::isValid() const
{
    return data->valid;
}

void QpDatasourceResult::invalidate()
{
    reset();
    setValid(false);
}

void QpDatasourceResult::setValid(bool valid)
{
    data->valid = valid;
}

int QpDatasourceResult::integerResult() const
{
    return data->integerResult;
}

void QpDatasourceResult::setIntegerResult(int result)
{
    data->integerResult = result;
    setValid(true);
}

QpError QpDatasourceResult::error() const
{
    return data->error;
}

void QpDatasourceResult::setError(const QpError &error)
{
    data->error = error;

    if(error.isValid()) {
        invalidate();
        data->dataAccessObject->storage()->setLastError(error);
    }
}

int QpDatasourceResult::size() const
{
    return data->records.size();
}

QList<QpDatasourceResult::Record> QpDatasourceResult::records() const
{
    return data->records;
}

void QpDatasourceResult::setRecords(const QList<Record> &records)
{
    data->records = records;
    setValid(true);
}

void QpDatasourceResult::addRecord(const Record &record)
{
    data->records.append(record);
    setValid(true);
}

void QpDatasourceResult::writeObjectProperties(QObject *object, const Record &record) const
{
    foreach(RecordField field, record.fields) {
        if(field.propertyIndex > 0) {
            record.metaObject.property(field.propertyIndex).write(object, field.value);
        }
        else{
            object->setProperty(field.name.toLatin1(), field.value);
        }
    }
}
