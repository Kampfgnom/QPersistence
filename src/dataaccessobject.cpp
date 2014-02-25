#include "dataaccessobject.h"

#include "cache.h"
#include "databaseschema.h"
#include "error.h"
#include "metaobject.h"
#include "metaproperty.h"
#include "qpersistence.h"
#include "sqlbackend.h"
#include "sqldataaccessobjecthelper.h"
#include "sqlquery.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSqlError>
#include <QSqlRecord>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpDaoBaseData : public QSharedData
{
public:
    QpDaoBaseData() :
        QSharedData(),
        count(-1)
    {
    }

    mutable int count;
    QpSqlDataAccessObjectHelper *sqlDataAccessObjectHelper;
    QpMetaObject metaObject;
    mutable QpError lastError;
    mutable QpCache cache;
};

typedef QHash<QString, QpDaoBase *> HashStringToDao;
QP_DEFINE_STATIC_LOCAL(HashStringToDao, DaoPerMetaObjectName)

QpDaoBase *QpDaoBase::forClass(const QMetaObject &metaObject)
{
    Q_ASSERT(DaoPerMetaObjectName()->contains(metaObject.className()));
    return DaoPerMetaObjectName()->value(metaObject.className());
}

QList<QpDaoBase *> QpDaoBase::dataAccessObjects()
{
    return DaoPerMetaObjectName()->values();
}

QpDaoBase::QpDaoBase(const QMetaObject &metaObject,
                     QObject *parent) :
    QObject(parent),
    data(new QpDaoBaseData)
{
    data->sqlDataAccessObjectHelper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    data->metaObject = QpMetaObject::registerMetaObject(metaObject);
    DaoPerMetaObjectName()->insert(metaObject.className(), this);
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

QList<QSharedPointer<QObject> > QpDaoBase::readAllObjects(int skip, int count, const QpSqlCondition &condition) const
{
    QpSqlQuery query = data->sqlDataAccessObjectHelper->readAllObjects(data->metaObject, skip, count, condition);

    if (data->sqlDataAccessObjectHelper->lastError().isValid()) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return QList<QSharedPointer<QObject> >();
    }

    QList<QSharedPointer<QObject> > result;
    result.reserve(count);
    QSqlRecord record = query.record();
    int index = record.indexOf(QLatin1String(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY));
    while(query.next()) {
        int key = query.value(index).toInt();

        QSharedPointer<QObject> currentObject;
        if (data->cache.contains(key)) {
            currentObject = data->cache.get(key);
        }

        if(!currentObject) {
            QObject *object = createInstance();
            currentObject = data->cache.insert(key, object);
            data->sqlDataAccessObjectHelper->readQueryIntoObject(query, record, object);
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
    count();

    ++data->count;

    emit objectCreated(obj);
    return obj;
}

Qp::UpdateResult QpDaoBase::updateObject(QSharedPointer<QObject> object)
{
    double databaseTime = Qp::Private::updateTimeInDatabase(object.data());
    double objectTime = Qp::Private::updateTimeInObject(object.data());

    if(databaseTime > objectTime)
        return Qp::UpdateConflict;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
    Q_ASSERT(qFuzzyCompare(databaseTime, objectTime));
#pragma clang diagnostic pop

    if (!data->sqlDataAccessObjectHelper->updateObject(data->metaObject, object.data())) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return Qp::UpdateError;
    }

    emit objectUpdated(object);
    return Qp::UpdateSuccess;
}

bool QpDaoBase::removeObject(QSharedPointer<QObject> object)
{
    if (!data->sqlDataAccessObjectHelper->removeObject(data->metaObject, object.data())) {
        setLastError(data->sqlDataAccessObjectHelper->lastError());
        return false;
    }
    count();

    --data->count;
    data->cache.remove(Qp::primaryKey(object));
    emit objectRemoved(object);
    return true;
}

Q_DECL_CONSTEXPR static inline bool qpFuzzyCompare(double p1, double p2) Q_REQUIRED_RESULT;
Q_DECL_CONSTEXPR static inline bool qpFuzzyCompare(double p1, double p2)
{
    return (qAbs(p1 - p2) * 10000000000000000. <= qMin(qAbs(p1), qAbs(p2)));
}

Qp::SynchronizeResult QpDaoBase::synchronizeObject(QSharedPointer<QObject> object)
{
    QObject *obj = object.data();
    double localTime = Qp::Private::updateTimeInObject(obj);
    double remoteTime = Qp::Private::updateTimeInDatabase(obj);

    if(qpFuzzyCompare(localTime, remoteTime))
        return Qp::Unchanged;

    int id = Qp::primaryKey(object);
    if (!data->sqlDataAccessObjectHelper->readObject(data->metaObject, id, obj)) {
        QpError error = data->sqlDataAccessObjectHelper->lastError();
        if(error.isValid())
            setLastError(error);

        return Qp::Error;
    }

    foreach(QpMetaProperty relation, QpMetaObject::forObject(object).relationProperties()) {
        QpRelationResolver::readRelationFromDatabase(relation, obj);
    }

    return Qp::Updated;
}

QList<QSharedPointer<QObject> > QpDaoBase::createdSince(const QDateTime &time)
{
    return readAllObjects(-1,-1, QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_CREATION_TIME,
                                                QpSqlCondition::GreaterThan,
                                                time.toString("yyyyMMddHHmmss.zzz").toDouble()));
}

QList<QSharedPointer<QObject> > QpDaoBase::updatedSince(const QDateTime &time)
{
    return readAllObjects(-1,-1, QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_UPDATE_TIME,
                                                QpSqlCondition::GreaterThan,
                                                time.toString("yyyyMMddHHmmss.zzz").toDouble()));
}

uint qHash(const QVariant &var)
{
    if (!var.isValid() || var.isNull())
        return 0;

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
    case QVariant::BitArray:
    case QVariant::Bitmap:
    case QVariant::Brush:
    case QVariant::Color:
    case QVariant::Cursor:
    case QVariant::EasingCurve:
    case QVariant::Font:
    case QVariant::Hash:
    case QVariant::Icon:
    case QVariant::Image:
    case QVariant::Invalid:
    case QVariant::KeySequence:
    case QVariant::LastCoreType:
    case QVariant::LastType:
    case QVariant::Line:
    case QVariant::LineF:
    case QVariant::List:
    case QVariant::Matrix4x4:
    case QVariant::Matrix:
    case QVariant::ModelIndex:
    case QVariant::Palette:
    case QVariant::Pen:
    case QVariant::Pixmap:
    case QVariant::Point:
    case QVariant::PointF:
    case QVariant::Polygon:
    case QVariant::PolygonF:
    case QVariant::Quaternion:
    case QVariant::Rect:
    case QVariant::RectF:
    case QVariant::Region:
    case QVariant::RegularExpression:
    case QVariant::Size:
    case QVariant::SizeF:
    case QVariant::SizePolicy:
    case QVariant::TextFormat:
    case QVariant::TextLength:
    case QVariant::Transform:
    case QVariant::UserType:
    case QVariant::Uuid:
    case QVariant::Vector2D:
    case QVariant::Vector3D:
    case QVariant::Vector4D:
    case QVariant::Map:
        Q_ASSERT(false);
    }

    // could not generate a hash for the given variant
    Q_ASSERT(false);
    return 0;
}
