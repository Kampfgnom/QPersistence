#include "datasourceresult.h"

#include "dataaccessobject.h"
#include "error.h"
#include "storage.h"

/******************************************************************************
 * QpDatasourceResultData
 */
class QpDatasourceResultData : public QSharedData
{
public:
    QpDatasourceResultData();

    const QpDataAccessObjectBase *dataAccessObject;

    bool valid;
    int integerResult;
    QpError error;
    QHash<int, QpDataTransferObject> dataTransferObjectsById;
    QList<QpDataTransferObject> dataTransferObjects;

    void invalidate();
    void setValid(bool valid);
    void reset();
};

QpDatasourceResultData::QpDatasourceResultData() :
    QSharedData()
{
    reset();
}

void QpDatasourceResultData::reset()
{
    valid = false;
    integerResult = -1;
    error = QpError();
    dataTransferObjects.clear();
    dataTransferObjectsById.clear();
}

void QpDatasourceResultData::invalidate()
{
    reset();
    setValid(false);
}

void QpDatasourceResultData::setValid(bool v)
{
    valid = v;
}

/******************************************************************************
 * QpDatasourceResult
 */
QpDatasourceResult::QpDatasourceResult(const QpDataAccessObjectBase *dao) :
    QObject(const_cast<QpDataAccessObjectBase *>(dao)),
    data(new QpDatasourceResultData)
{
    data->dataAccessObject = dao;
}

QpDatasourceResult::~QpDatasourceResult()
{
}

bool QpDatasourceResult::isValid() const
{
    return data->valid && !data->error.isValid();
}

int QpDatasourceResult::integerResult() const
{
    return data->integerResult;
}

void QpDatasourceResult::setIntegerResult(int result)
{
    data->integerResult = result;
}

QpError QpDatasourceResult::lastError() const
{
    return data->error;
}

void QpDatasourceResult::setLastError(const QpError &e)
{
    data->error = e;

    if(e.isValid()) {
        data->invalidate();
        data->dataAccessObject->storage()->setLastError(e);
        emit error(e);
    }
}

int QpDatasourceResult::size() const
{
    return data->dataTransferObjects.size();
}

QList<QpDataTransferObject> QpDatasourceResult::dataTransferObjects() const
{
    return data->dataTransferObjects;
}

QpDataTransferObjectsById QpDatasourceResult::dataTransferObjectsById() const
{
    return data->dataTransferObjectsById;
}

void QpDatasourceResult::setDataTransferObjects(const QpDataTransferObjectsById &dataTransferObjects)
{
    data->dataTransferObjects = dataTransferObjects.values();
    data->dataTransferObjectsById = dataTransferObjects;
}

void QpDatasourceResult::addDataTransferObject(const QpDataTransferObject &dataTransferObject)
{
    data->dataTransferObjects << dataTransferObject;
    data->dataTransferObjectsById.insert(dataTransferObject.primaryKey, dataTransferObject);
}

void QpDatasourceResult::finish()
{
    data->setValid(true);
    emit finished();
}

void QpDatasourceResult::reset()
{
    data->reset();
}

QpDataTransferObject::QpDataTransferObject() :
    primaryKey(0)
{
}

void QpDataTransferObject::write(QObject *object) const
{
    object->setProperty("_Qp_dataTransferObject", QVariant::fromValue<QpDataTransferObject>(*this));

    foreach(int propertyIndex, properties.keys()) {
        metaObject.property(propertyIndex).write(object, properties.value(propertyIndex));
    }

    foreach(QString propertyName, dynamicProperties.keys()) {
        object->setProperty(propertyName.toLatin1(), dynamicProperties.value(propertyName));
    }
}

QpDataTransferObject QpDataTransferObject::fromObject(const QObject *object)
{
    return object->property("_Qp_dataTransferObject").value<QpDataTransferObject>();
}
