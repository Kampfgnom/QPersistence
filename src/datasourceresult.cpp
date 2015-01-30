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
    QHash<int, QpDataTransferObject> dataTransferObjectsById;
    QList<QpDataTransferObject> dataTransferObjects;
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
    data->dataTransferObjects.clear();
    data->dataTransferObjectsById.clear();
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
    return data->dataTransferObjects.size();
}

QList<QpDataTransferObject> QpDatasourceResult::dataTransferObjects() const
{
    return data->dataTransferObjects;
}

QHash<int, QpDataTransferObject> QpDatasourceResult::dataTransferObjectsById() const
{
    return data->dataTransferObjectsById;
}

void QpDatasourceResult::setDataTransferObjects(const QHash<int, QpDataTransferObject> &dataTransferObjects)
{
    data->dataTransferObjects = dataTransferObjects.values();
    data->dataTransferObjectsById = dataTransferObjects;
}

void QpDatasourceResult::addDataTransferObject(const QpDataTransferObject &dataTransferObject)
{
    data->dataTransferObjects << dataTransferObject;
    data->dataTransferObjectsById.insert(dataTransferObject.primaryKey, dataTransferObject);
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
