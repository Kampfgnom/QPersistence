#include "conversion.h"

#include "qpersistence.h"

namespace Qp {

namespace Private {

typedef QHash<int, ConverterBase *> HashIntToToConverter;
typedef QHash<QString, ConverterBase *> HashStringToConverter;
QP_DEFINE_STATIC_LOCAL(HashIntToToConverter, ConvertersByUserType)
QP_DEFINE_STATIC_LOCAL(HashStringToConverter, ConvertersByClassName)

bool canConvertToSqlStorableVariant(const QVariant &variant)
{
    return Private::ConvertersByUserType()->contains(variant.userType());
}

QString convertToSqlStorableVariant(const QVariant &variant)
{
    if (!Private::ConvertersByUserType()->contains(variant.userType()))
        return variant.toString();

    return Private::ConvertersByUserType()->value(variant.userType())->convertToSqlStorableValue(variant);
}

bool canConvertFromSqlStoredVariant(QMetaType::Type type)
{
    return Private::ConvertersByUserType()->contains(type);
}

QVariant convertFromSqlStoredVariant(const QString &variant, QMetaType::Type type)
{
    if (!Private::ConvertersByUserType()->contains(type))
        return variant;

    return Private::ConvertersByUserType()->value(type)->convertFromSqlStorableValue(variant);
}

void registerConverter(int variantType, ConverterBase *converter)
{
    ConvertersByUserType()->insert(variantType, converter);
    ConvertersByClassName()->insert(converter->className(), converter);
}

QSharedPointer<QObject> objectCast(const QVariant &variant)
{
    if(!variant.isValid() || variant.isNull() || !ConvertersByUserType()->contains(variant.userType()))
        return QSharedPointer<QObject>();

    if(Qp::variantUserType<QObject>() == variant.userType())
        return variant.value<QSharedPointer<QObject> >();

    return ConvertersByUserType()->value(variant.userType())->convertObject(variant);
}

QList<QSharedPointer<QObject> > objectListCast(const QVariant &variant)
{
    if(!variant.isValid() || variant.isNull())
        return {};

    Q_ASSERT(ConvertersByUserType()->contains(variant.userType()));

    return ConvertersByUserType()->value(variant.userType())->convertList(variant);
}

QVariant variantCast(QSharedPointer<QObject> object, const QString &classN)
{
    QString className = classN;
    if (className.isEmpty()) {
        Q_ASSERT(object);
        className = QLatin1String(object->metaObject()->className());
    }
    Q_ASSERT(ConvertersByClassName()->contains(className));
    return ConvertersByClassName()->value(className)->convertVariant(object);
}

QVariant variantListCast(QList<QSharedPointer<QObject> > objects, const QString &className)
{
    Q_ASSERT(ConvertersByClassName()->contains(className));
    return ConvertersByClassName()->value(className)->convertVariantList(objects);
}

QList<QSharedPointer<QObject> > ConverterBase::convertList(const QVariant &variant) const { Q_UNUSED(variant) return QList<QSharedPointer<QObject> >(); }
QSharedPointer<QObject> ConverterBase::convertObject(const QVariant &variant) const { Q_UNUSED(variant) return QSharedPointer<QObject>(); }
QVariant ConverterBase::convertVariant(QSharedPointer<QObject> object) const { Q_UNUSED(object) return QVariant(); }
QVariant ConverterBase::convertVariantList(QList<QSharedPointer<QObject> > objects) const { Q_UNUSED(objects) return QVariant(); }
QString ConverterBase::className() const { return QString(); }
QString ConverterBase::convertToSqlStorableValue(const QVariant &variant) const { Q_UNUSED(variant) return QString(); }
QVariant ConverterBase::convertFromSqlStorableValue(const QString &value) const { Q_UNUSED(value) return QVariant(); }

}

}
