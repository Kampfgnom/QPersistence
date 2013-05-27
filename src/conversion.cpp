#include "conversion.h"

namespace Qp {

namespace Private {

QHash<int, ConverterBase *> convertersByUserType;
QHash<QString, ConverterBase *> convertersByClassName;

bool canConvertToSqlStorableVariant(const QVariant &variant)
{
    return Private::convertersByUserType.contains(variant.userType());
}

QString convertToSqlStorableVariant(const QVariant &variant)
{
    if(!Private::convertersByUserType.contains(variant.userType()))
        return variant.toString();

    return Private::convertersByUserType.value(variant.userType())->convertToSqlStorableValue(variant);
}

bool canConvertFromSqlStoredVariant(QMetaType::Type type)
{
    return Private::convertersByUserType.contains(type);
}

QVariant convertFromSqlStoredVariant(const QString &variant, QMetaType::Type type)
{
    if(!Private::convertersByUserType.contains(type))
        return variant;

    return Private::convertersByUserType.value(type)->convertFromSqlStorableValue(variant);
}

void registerConverter(int variantType, ConverterBase *converter)
{
    convertersByUserType.insert(variantType, converter);
    convertersByClassName.insert(converter->className(), converter);
}

QSharedPointer<QObject> objectCast(const QVariant &variant)
{
    Q_ASSERT(convertersByUserType.contains(variant.userType()));

    return convertersByUserType.value(variant.userType())->convertObject(variant);
}

QList<QSharedPointer<QObject> > objectListCast(const QVariant &variant)
{
    Q_ASSERT(convertersByUserType.contains(variant.userType()));

    return convertersByUserType.value(variant.userType())->convertList(variant);
}

QVariant variantCast(QSharedPointer<QObject> object, const QString &classN)
{
    QString className = classN;
    if(className.isEmpty()) {
        Q_ASSERT(object);
        className = QLatin1String(object->metaObject()->className());
    }
    Q_ASSERT(convertersByClassName.contains(className));
    return convertersByClassName.value(className)->convertVariant(object);
}

QVariant variantListCast(QList<QSharedPointer<QObject> > objects, const QString &className)
{
    Q_ASSERT(convertersByClassName.contains(className));
    return convertersByClassName.value(className)->convertVariantList(objects);
}

}

}
