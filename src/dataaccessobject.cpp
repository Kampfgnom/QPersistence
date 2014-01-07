#include "dataaccessobject.h"

#include "error.h"
#include "metaobject.h"
#include "sqldataaccessobjecthelper.h"
#include "qpersistence.h"
#include "cache.h"
#include "metaproperty.h"

#include <QtCore/QVariant>

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
    d(new QpDaoBaseData)
{
    d->sqlDataAccessObjectHelper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    d->metaObject = QpMetaObject::registerMetaObject(metaObject);
    QpDaoBaseData::daoPerMetaObjectName.insert(metaObject.className(), this);
}

QpDaoBase::~QpDaoBase()
{
}

QpError QpDaoBase::lastError() const
{
    return d->lastError;
}

void QpDaoBase::setLastError(const QpError &error) const
{
    d->lastError = error;
}

void QpDaoBase::resetLastError() const
{
    setLastError(QpError());
}

QpSqlDataAccessObjectHelper *QpDaoBase::sqlDataAccessObjectHelper() const
{
    return d->sqlDataAccessObjectHelper;
}

QpMetaObject QpDaoBase::qpMetaObject() const
{
    return d->metaObject;
}

int QpDaoBase::count() const
{
    if(d->count < 0)
        d->count = d->sqlDataAccessObjectHelper->count(d->metaObject);;

    return d->count;
}

QList<int> QpDaoBase::allKeys(int skip, int count) const
{
    QList<int> result = d->sqlDataAccessObjectHelper->allKeys(d->metaObject, skip, count);

    if(d->sqlDataAccessObjectHelper->lastError().isValid())
        setLastError(d->sqlDataAccessObjectHelper->lastError());

    return result;
}

QList<QSharedPointer<QObject> > QpDaoBase::readAllObjects(int skip, int count) const
{
    int myCount = this->count();

    if(d->cache.size() == myCount)
        return d->cache.objects(skip,count);

    if(count <= 0)
        count = myCount;

    QList<QObject *> objects;

    for(int i = 0; i < count; ++i)
        objects.append(createInstance());

    if(!d->sqlDataAccessObjectHelper->readAllObjects(d->metaObject, objects, skip, count)) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        for(int i = 0; i < count; ++i)
            delete objects.at(i);

        return QList<QSharedPointer<QObject> >();
    }

    QList<QSharedPointer<QObject> > result;
    result.reserve(count);
    for(int i = 0; i < count; ++i) {
        QObject *object = objects.at(i);
        int key = Qp::Private::primaryKey(object);
        if(d->cache.contains(key)) {
            result.append(d->cache.get(key));
            delete object;
        }
        else {
            QSharedPointer<QObject> obj = d->cache.insert(Qp::Private::primaryKey(object), object);
            Qp::Private::enableSharedFromThis(obj);
            result.append(obj);
        }
    }

    return result;
}

QSharedPointer<QObject> QpDaoBase::readObject(int id) const
{
    QSharedPointer<QObject> p = d->cache.get(id);

    if(p)
        return p;

    QObject *object = createInstance();

    if(!d->sqlDataAccessObjectHelper->readObject(d->metaObject, id, object)) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        delete object;
        return QSharedPointer<QObject>();
    }
    QSharedPointer<QObject> obj = d->cache.insert(Qp::Private::primaryKey(object), object);
    Qp::Private::enableSharedFromThis(obj);

    return obj;
}

QSharedPointer<QObject> QpDaoBase::createObject()
{
    QObject *object = createInstance();

    if(!d->sqlDataAccessObjectHelper->insertObject(d->metaObject, object)) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        return QSharedPointer<QObject>();
    }
    QSharedPointer<QObject> obj = d->cache.insert(Qp::Private::primaryKey(object), object);
    Qp::Private::enableSharedFromThis(obj);
    ++d->count;

    emit objectCreated(obj);
    return obj;
}

bool QpDaoBase::updateObject(QSharedPointer<QObject> object)
{
    if(!d->sqlDataAccessObjectHelper->updateObject(d->metaObject, object.data())) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        return false;
    }

    emit objectUpdated(object);
    return true;
}

bool QpDaoBase::removeObject(QSharedPointer<QObject> object)
{
    if(!d->sqlDataAccessObjectHelper->removeObject(d->metaObject, object.data())) {
        setLastError(d->sqlDataAccessObjectHelper->lastError());
        return false;
    }

    --d->count;
    d->cache.remove(Qp::primaryKey(object));
    emit objectRemoved(object);
    return true;
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
