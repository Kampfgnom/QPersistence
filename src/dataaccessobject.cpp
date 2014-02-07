#include "dataaccessobject.h"

#include "cache.h"
#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqldataaccessobjecthelper.h"
#include "sqlquery.h"

#include <QSqlError>

class QpDaoBaseData : public QSharedData
{
public:
    QpDaoBaseData() :
        QSharedData(),
        count(-1)
    {
    }

    QpSqlDataAccessObjectHelper *sqlDataAccessObjectHelper;
    QpMetaObject metaObject;
    mutable QpError lastError;
    mutable QpCache cache;
    mutable int count;

    static QHash<QString, QpDaoBase *> daoPerMetaObjectName;
};

QHash<QString, QpDaoBase *> QpDaoBaseData::daoPerMetaObjectName;

QpDaoBase *QpDaoBase::forClass(const QMetaObject &metaObject)
{
    Q_ASSERT(QpDaoBaseData::daoPerMetaObjectName.contains(metaObject.className()));
    return QpDaoBaseData::daoPerMetaObjectName.value(metaObject.className());
}

QList<QpDaoBase *> QpDaoBase::dataAccessObjects()
{
    return QpDaoBaseData::daoPerMetaObjectName.values();
}

QpDaoBase::QpDaoBase(const QMetaObject &metaObject,
                     QObject *parent) :
    QObject(parent),
    data(new QpDaoBaseData)
{
    data->sqlDataAccessObjectHelper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    data->metaObject = QpMetaObject::registerMetaObject(metaObject);
    QpDaoBaseData::daoPerMetaObjectName.insert(metaObject.className(), this);
}

QpDaoBase::~QpDaoBase()
{
}

QpError QpDaoBase::lastError() const
{
    return data->lastError;
}

void QpDaoBase::setLastError(const QpError &error) const
{
    data->lastError = error;
    Qp::Private::setLastError(error);
}

void QpDaoBase::resetLastError() const
{
    setLastError(QpError());
}

QpSqlDataAccessObjectHelper *QpDaoBase::sqlDataAccessObjectHelper() const
{
    return data->sqlDataAccessObjectHelper;
}

QpMetaObject QpDaoBase::qpMetaObject() const
{
    return data->metaObject;
}

int QpDaoBase::count() const
{
    if (data->count < 0)
        data->count = data->sqlDataAccessObjectHelper->count(data->metaObject);;

    return data->count;
}

QList<int> QpDaoBase::allKeys(int skip, int count) const
{
    QList<int> result = data->sqlDataAccessObjectHelper->allKeys(data->metaObject, skip, count);

    if (data->sqlDataAccessObjectHelper->lastError().isValid())
        setLastError(data->sqlDataAccessObjectHelper->lastError());

    return result;
}

QList<QSharedPointer<QObject> > QpDaoBase::readAllObjects(int skip, int count) const
{
    int myCount = this->count();

    if (count <= 0 && myCount != 0)
        count = myCount;

    QpSqlQuery query = data->sqlDataAccessObjectHelper->readAllObjects(data->metaObject, skip, count);

    if (data->sqlDataAccessObjectHelper->lastError().isValid()) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return QList<QSharedPointer<QObject> >();
    }

    QList<QSharedPointer<QObject> > result;
    result.reserve(count);
    while(query.next()) {
        int key = query.value(QLatin1String("_Qp_ID")).toInt();

        QSharedPointer<QObject> currentObject;
        if (data->cache.contains(key)) {
            currentObject = data->cache.get(key);
        }

        if(!currentObject) {
            QObject *object = createInstance();
            currentObject = data->cache.insert(key, object);
            data->sqlDataAccessObjectHelper->readQueryIntoObject(query, object);
            Qp::Private::enableSharedFromThis(currentObject);
        }

        result.append(currentObject);
    }

    if (data->sqlDataAccessObjectHelper->lastError().isValid()) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return QList<QSharedPointer<QObject> >();
    }

    return result;
}

QSharedPointer<QObject> QpDaoBase::readObject(int id) const
{
    QSharedPointer<QObject> p = data->cache.get(id);

    if (p)
        return p;

    QObject *object = createInstance();

    if (!data->sqlDataAccessObjectHelper->readObject(data->metaObject, id, object)) {
        QpError error = data->sqlDataAccessObjectHelper->lastError();
        if(error.isValid())
            setLastError(error);

        delete object;
        return QSharedPointer<QObject>();
    }
    QSharedPointer<QObject> obj = data->cache.insert(Qp::Private::primaryKey(object), object);
    Qp::Private::enableSharedFromThis(obj);

    return obj;
}

QSharedPointer<QObject> QpDaoBase::createObject()
{
    QObject *object = createInstance();

    if (!data->sqlDataAccessObjectHelper->insertObject(data->metaObject, object)) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return QSharedPointer<QObject>();
    }
    QSharedPointer<QObject> obj = data->cache.insert(Qp::Private::primaryKey(object), object);
    Qp::Private::enableSharedFromThis(obj);
    ++data->count;

    emit objectCreated(obj);
    return obj;
}

bool QpDaoBase::updateObject(QSharedPointer<QObject> object)
{
    if (!data->sqlDataAccessObjectHelper->updateObject(data->metaObject, object.data())) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return false;
    }

    emit objectUpdated(object);
    return true;
}

bool QpDaoBase::removeObject(QSharedPointer<QObject> object)
{
    if (!data->sqlDataAccessObjectHelper->removeObject(data->metaObject, object.data())) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return false;
    }

    --data->count;
    data->cache.remove(Qp::primaryKey(object));
    emit objectRemoved(object);
    return true;
}

Qp::SynchronizeResult QpDaoBase::synchronizeObject(QSharedPointer<QObject> object)
{
    QDateTime localTime = Qp::Private::updateTimeInObject(object.data());
    QDateTime remoteTime = Qp::Private::updateTimeInDatabase(object.data());

    if(localTime == remoteTime)
        return Qp::Unchanged;

    int id = Qp::primaryKey(object);
    if (!data->sqlDataAccessObjectHelper->readObject(data->metaObject, id, object.data())) {
        QpError error = data->sqlDataAccessObjectHelper->lastError();
        if(error.isValid())
            setLastError(error);

        return Qp::Error;
    }

    return Qp::Updated;
}

uint qHash(const QVariant &var)
{
    if (!var.isValid() || var.isNull())
        return -1;

    switch (var.type())
    {
    case QVariant::Int:
        return qHash( var.toInt() );
    case QVariant::UInt:
        return qHash( var.toUInt() );
    case QVariant::Bool:
        return qHash( var.toUInt() );
    case QVariant::Double:
        return qHash( var.toUInt() );
    case QVariant::LongLong:
        return qHash( var.toLongLong() );
    case QVariant::ULongLong:
        return qHash( var.toULongLong() );
    case QVariant::String:
        return qHash( var.toString() );
    case QVariant::Char:
        return qHash( var.toChar() );
    case QVariant::StringList:
        return qHash( var.toString() );
    case QVariant::ByteArray:
        return qHash( var.toByteArray() );
    case QVariant::Date:
    case QVariant::Time:
    case QVariant::DateTime:
    case QVariant::Url:
    case QVariant::Locale:
    case QVariant::RegExp:
        return qHash( var.toString() );
    case QVariant::Map:
    case QVariant::List:
    case QVariant::BitArray:
    case QVariant::Size:
    case QVariant::SizeF:
    case QVariant::Rect:
    case QVariant::LineF:
    case QVariant::Line:
    case QVariant::RectF:
    case QVariant::Point:
    case QVariant::UserType:
    case QVariant::Invalid:
    default:
        Q_ASSERT(false);
    }

    // could not generate a hash for the given variant
    Q_ASSERT(false);
    return 0;
}
