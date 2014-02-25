#include "sqlbackend.h"

#include "private.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QHash>
#include <QSharedData>
#include <QSqlDatabase>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpSqlBackendData
{
public:
    static QpSqlBackend *createForDatabase(const QSqlDatabase &database);
};

typedef QHash<QString, QpSqlBackend *> HashStringToBackend;
QP_DEFINE_STATIC_LOCAL(HashStringToBackend, Backends)

QpSqlBackend *QpSqlBackendData::createForDatabase(const QSqlDatabase &database)
{
    if(database.driverName() == QLatin1String("QSQLITE"))
        return new QpSqliteBackend(Qp::Private::GlobalGuard());
    if(database.driverName() == QLatin1String("QMYSQL"))
        return new QpMySqlBackend(Qp::Private::GlobalGuard());


    Q_ASSERT_X(false, Q_FUNC_INFO,
               QString("No backend for database '%1'")
               .arg(database.driverName())
               .toLatin1());

    return nullptr; // Never reached
}

QpSqlBackend *QpSqlBackend::forDatabase(const QSqlDatabase &database)
{
    auto it = Backends()->find(database.driverName());

    if(it != Backends()->end())
        return it.value();

    QpSqlBackend *backend = QpSqlBackendData::createForDatabase(database);
    Backends()->insert(database.driverName(),backend);
    return backend;
}

QpSqlBackend::QpSqlBackend(QObject *parent) :
    QObject(parent)
{
}

QpSqlBackend::~QpSqlBackend()
{
}

QpSqliteBackend::QpSqliteBackend(QObject *parent) :
    QpSqlBackend(parent)
{
}

QString QpSqliteBackend::primaryKeyType() const
{
    return QLatin1String("INTEGER PRIMARY KEY AUTOINCREMENT");
}

QString QpSqliteBackend::variantTypeToSqlType(QVariant::Type type) const
{
    switch (type) {
    case QVariant::UInt:
    case QVariant::Int:
    case QVariant::Bool:
    case QVariant::ULongLong:
        return QLatin1String("INTEGER");
    case QVariant::String:
    case QVariant::StringList:
    case QVariant::Date:
    case QVariant::DateTime:
    case QVariant::Time:
    case QVariant::Char:
    case QVariant::Url:
        return QLatin1String("TEXT");
    case QVariant::Double:
        return QLatin1String("REAL");
    case QVariant::BitArray:
    case QVariant::Bitmap:
    case QVariant::Brush:
    case QVariant::ByteArray:
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
    case QVariant::Locale:
    case QVariant::LongLong:
    case QVariant::Map:
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
    case QVariant::RegExp:
    case QVariant::Region:
    case QVariant::RegularExpression:
    case QVariant::Size:
    case QVariant::SizeF:
    case QVariant::SizePolicy:
    case QVariant::TextFormat:
    case QVariant::TextLength:
    case QVariant::Transform:
    case QVariant::Uuid:
    case QVariant::Vector2D:
    case QVariant::Vector3D:
    case QVariant::Vector4D:
    case QVariant::UserType:
        return QLatin1String("BLOB");
    }

    Q_ASSERT(false);
    return QString();
}

QpMySqlBackend::QpMySqlBackend(QObject *parent) :
    QpSqlBackend(parent)
{
}

QString QpMySqlBackend::primaryKeyType() const
{
    return QLatin1String("INTEGER PRIMARY KEY AUTO_INCREMENT");
}

QString QpMySqlBackend::variantTypeToSqlType(QVariant::Type type) const
{
    switch (type) {
    case QVariant::UInt:
    case QVariant::Int:
        return QLatin1String("INTEGER");
    case QVariant::Bool:
    case QVariant::ULongLong:
        return QLatin1String("BIGINT");
    case QVariant::String:
    case QVariant::StringList:
    case QVariant::Url:
        return QLatin1String("TEXT");
    case QVariant::Date:
        return QLatin1String("DATE");
    case QVariant::DateTime:
        return QLatin1String("DATETIME");
    case QVariant::Time:
        return QLatin1String("TIMESTAMP");
    case QVariant::Char:
        return QLatin1String("CHAR");
    case QVariant::Double:
        return QLatin1String("DOUBLE");
    case QVariant::BitArray:
    case QVariant::Bitmap:
    case QVariant::Brush:
    case QVariant::ByteArray:
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
    case QVariant::Locale:
    case QVariant::LongLong:
    case QVariant::Map:
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
    case QVariant::RegExp:
    case QVariant::Region:
    case QVariant::RegularExpression:
    case QVariant::Size:
    case QVariant::SizeF:
    case QVariant::SizePolicy:
    case QVariant::TextFormat:
    case QVariant::TextLength:
    case QVariant::Transform:
    case QVariant::Uuid:
    case QVariant::Vector2D:
    case QVariant::Vector3D:
    case QVariant::Vector4D:
    case QVariant::UserType:
        return QLatin1String("BLOB");
    }

    Q_ASSERT(false);
    return QString();
}


QString QpSqliteBackend::nowTimestamp() const
{
    return QLatin1String("now");
}

QString QpMySqlBackend::nowTimestamp() const
{
    return QLatin1String("NOW() + 0");
}
