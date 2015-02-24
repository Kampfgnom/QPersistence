#include "datasourceresult.h"

#include "dataaccessobject.h"
#include "error.h"
#include "metaproperty.h"
#include "relations.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSet>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

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

    if (e.isValid()) {
        data->invalidate();
        data->dataAccessObject->storage()->setLastError(e);
        emit error(e);
    }
}

int QpDatasourceResult::size() const
{
    return data->dataTransferObjects.size();
}

bool QpDatasourceResult::isEmpty() const
{
    return data->dataTransferObjects.isEmpty();
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

int QpDataTransferObject::revision() const
{
    return dynamicProperties.value(QpDatabaseSchema::COLUMN_NAME_REVISION).toInt();
}

bool QpDataTransferObject::isEmpty() const
{
    return properties.isEmpty() && dynamicProperties.isEmpty() && toOneRelationFKs.isEmpty() && toManyRelationFKs.isEmpty();
}

QpDataTransferObjectDiff QpDataTransferObject::diff(const QObject *object) const
{
    QpDataTransferObjectDiff result;

    QpDataTransferObject previousDto = QpDataTransferObject::fromObject(object);
    if (previousDto.isEmpty()) {
        result.right = *this;
        return result;
    }

    if (previousDto.revision() == revision()) {
        result.right = *this;
        return result;
    }

    QpDataTransferObject objectDto = QpDataTransferObject::readObject(object);
    result.left = previousDto.compare(objectDto);
    result.right = previousDto.compare(*this);

    foreach (int propertyIndex, previousDto.properties.keys()) {
        if (!result.left.properties.contains(propertyIndex)
            || !result.right.properties.contains(propertyIndex))
            continue;

        QVariant leftValue = result.left.properties.value(propertyIndex);
        QVariant rightValue = result.right.properties.value(propertyIndex);
        if (leftValue != rightValue) {
            result.left.properties.remove(propertyIndex);
            result.right.properties.remove(propertyIndex);
            result.conflictsLeft.properties.insert(propertyIndex, leftValue);
            result.conflictsRight.properties.insert(propertyIndex, rightValue);
        }
    }

    foreach (QString propertyName, previousDto.dynamicProperties.keys()) {
        if (!result.left.dynamicProperties.contains(propertyName)
            || !result.right.dynamicProperties.contains(propertyName))
            continue;

        QVariant leftValue = result.left.dynamicProperties.value(propertyName);
        QVariant rightValue = result.right.dynamicProperties.value(propertyName);
        if (leftValue != rightValue) {
            result.left.dynamicProperties.remove(propertyName);
            result.right.dynamicProperties.remove(propertyName);
            result.conflictsLeft.dynamicProperties.insert(propertyName, leftValue);
            result.conflictsRight.dynamicProperties.insert(propertyName, rightValue);
        }
    }

    foreach (int propertyIndex, previousDto.toOneRelationFKs.keys()) {
        if (!result.left.toOneRelationFKs.contains(propertyIndex)
            || !result.right.toOneRelationFKs.contains(propertyIndex))
            continue;

        int leftValue = result.left.toOneRelationFKs.value(propertyIndex);
        int rightValue = result.right.toOneRelationFKs.value(propertyIndex);
        if (leftValue != rightValue) {
            result.left.toOneRelationFKs.remove(propertyIndex);
            result.right.toOneRelationFKs.remove(propertyIndex);
            result.conflictsLeft.toOneRelationFKs.insert(propertyIndex, leftValue);
            result.conflictsRight.toOneRelationFKs.insert(propertyIndex, rightValue);
        }
    }

    // to-many relations are never in conflict by definition

    return result;
}

QpDataTransferObject QpDataTransferObject::compare(const QpDataTransferObject &other) const
{
    QpDataTransferObject result;
    result.primaryKey = other.primaryKey;
    result.metaObject = other.metaObject;

    foreach (int propertyIndex, other.properties.keys()) {
        QVariant otherValue = other.properties.value(propertyIndex);
        if (properties.value(propertyIndex) != otherValue)
            result.properties.insert(propertyIndex, otherValue);
    }

    foreach (QString propertyName, other.dynamicProperties.keys()) {
        QVariant otherValue = other.dynamicProperties.value(propertyName);
        if (dynamicProperties.value(propertyName) != otherValue)
            result.dynamicProperties.insert(propertyName, otherValue);
    }

    foreach (int propertyIndex, other.toOneRelationFKs.keys()) {
        int otherValue = other.toOneRelationFKs.value(propertyIndex);
        if (toOneRelationFKs.value(propertyIndex) != otherValue)
            result.toOneRelationFKs.insert(propertyIndex, otherValue);
    }

    foreach (int propertyIndex, other.toManyRelationFKs.keys()) {
        const QList<int> fks = toManyRelationFKs.value(propertyIndex);
        const QList<int> otherFks = other.toManyRelationFKs.value(propertyIndex);

        const QList<int> added = otherFks.toSet().subtract(fks.toSet()).toList();
        if (!added.isEmpty())
            result.toManyRelationFKsAdded.insert(propertyIndex, added);

        const QList<int> removed = fks.toSet().subtract(otherFks.toSet()).toList();
        if (!removed.isEmpty())
            result.toManyRelationFKsRemoved.insert(propertyIndex, removed);
    }

    return result;
}

QpDataTransferObject QpDataTransferObject::merge(const QpDataTransferObject &other) const
{
    QpDataTransferObject result = *this;

    foreach (int propertyIndex, other.properties.keys()) {
        result.properties.insert(propertyIndex, other.properties.value(propertyIndex));
    }

    foreach (QString propertyName, other.dynamicProperties.keys()) {
        result.dynamicProperties.insert(propertyName, other.dynamicProperties.value(propertyName));
    }

    foreach (int propertyIndex, other.toOneRelationFKs.keys()) {
        result.toOneRelationFKs.insert(propertyIndex, other.toOneRelationFKs.value(propertyIndex));
    }

    foreach (int propertyIndex, other.toManyRelationFKs.keys()) {
        QSet<int> merged = toManyRelationFKs.value(propertyIndex).toSet();
        const QSet<int> added = other.toManyRelationFKsAdded.value(propertyIndex).toSet();
        const QSet<int> removed = other.toManyRelationFKsRemoved.value(propertyIndex).toSet();
        merged.unite(added);
        merged.subtract(removed);
        result.toManyRelationFKs.insert(propertyIndex, merged.toList());
        result.toManyRelationFKsAdded.insert(propertyIndex, added.toList());
        result.toManyRelationFKsRemoved.insert(propertyIndex, removed.toList());
    }

    return result;
}

void QpDataTransferObject::write(QObject *object) const
{
    foreach (int propertyIndex, properties.keys()) {
        metaObject.property(propertyIndex).write(object, properties.value(propertyIndex));
    }

    foreach (QString propertyName, dynamicProperties.keys()) {
        object->setProperty(propertyName.toLatin1(), dynamicProperties.value(propertyName));
    }

    QpMetaObject qpmo = QpMetaObject::forObject(object);
    foreach (QpMetaProperty relation, qpmo.relationProperties()) {
        if (!toManyRelationFKs.contains(relation.metaProperty().propertyIndex())
            && !toOneRelationFKs.contains(relation.metaProperty().propertyIndex())
            && !toManyRelationFKsAdded.contains(relation.metaProperty().propertyIndex())
            && !toManyRelationFKsRemoved.contains(relation.metaProperty().propertyIndex()))
            continue;
        QpRelationBase *rb = relation.internalRelationObject(object);
        rb->adjustFromDataTransferObject(*this);
    }

    object->setProperty("_Qp_dataTransferObject", QVariant::fromValue<QpDataTransferObject>(*this));
}

QpDataTransferObjectDiff QpDataTransferObject::rebase(QObject *object) const
{
    QpDataTransferObjectDiff diff = QpDataTransferObject::diff(object);
    if (diff.left.isEmpty()) {
        diff.right.write(object);
        return diff;
    }

    if (diff.isConflict())
        return diff;

    QpDataTransferObject merged = diff.right.merge(diff.left);
    merged.write(object);
    return diff;
}

QpDataTransferObject QpDataTransferObject::fromObject(const QObject *object)
{
    return object->property("_Qp_dataTransferObject").value<QpDataTransferObject>();
}

QpDataTransferObject QpDataTransferObject::readObject(const QObject *object)
{
    QpDataTransferObject result;
    const QMetaObject *mo = object->metaObject();
    result.metaObject = *mo;
    result.primaryKey = Qp::Private::primaryKey(object);

    QpMetaObject qpmo = QpMetaObject::forMetaObject(result.metaObject);
    foreach (QpMetaProperty metaProperty, qpmo.simpleProperties()) {
        QMetaProperty property = metaProperty.metaProperty();
        result.properties.insert(property.propertyIndex(), property.read(object));
    }

    foreach (QByteArray p, object->dynamicPropertyNames()) {
        result.dynamicProperties.insert(QString::fromLatin1(p), object->property(p));
    }

    foreach (QpMetaProperty relation, qpmo.relationProperties()) {
        QpRelationBase *rb = relation.internalRelationObject(object);
        if (relation.isToManyRelationProperty()) {
            QpRelationToManyBase *toMany = static_cast<QpRelationToManyBase *>(rb);
            result.toManyRelationFKs.insert(relation.metaProperty().propertyIndex(), toMany->foreignKeys());
        }
        else {
            QpRelationToOneBase *toOne = static_cast<QpRelationToOneBase *>(rb);
            result.toOneRelationFKs.insert(relation.metaProperty().propertyIndex(), toOne->foreignKey());
        }
    }

    return result;
}

bool QpDataTransferObjectDiff::isEmpty() const
{
    return left.isEmpty() && right.isEmpty();
}

bool QpDataTransferObjectDiff::isConflict() const
{
    // Right and left conflicts must always be in sync, so we won't have to test conflictsRight for emptyness
    return !conflictsLeft.properties.isEmpty()
           || !conflictsLeft.dynamicProperties.isEmpty()
           || !conflictsLeft.toOneRelationFKs.isEmpty();
}
