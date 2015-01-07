#include "relation_hasmany.h"
#include <QSharedData>

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"


/******************************************************************************
 * QpHasManyBase
 */
class QpHasManyData : public QSharedData {
public:
    QpHasManyData() :
        QSharedData(),
        resolved(false),
        parent(nullptr)
    {
    }

    bool resolved;
    QList<QSharedPointer<QObject>> objects;
    QpMetaProperty metaProperty;
    QObject *parent;
};


/******************************************************************************
 * QpHasManyBase
 */
QpHasManyBase::QpHasManyBase(const QString &name, QObject *parent) :
    data(new QpHasManyData)
{
    data->parent = parent;
    QString propertyName = QpMetaProperty::nameFromMaybeQualifiedName(name);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(propertyName);
}

QpHasManyBase::~QpHasManyBase()
{
}

bool QpHasManyBase::operator ==(const QList<QSharedPointer<QObject> > &objects) const
{
    return data->objects == objects;
}

QList<QSharedPointer<QObject> > QpHasManyBase::objects() const
{
    if (Qp::Private::primaryKey(data->parent) == 0)
        return QList<QSharedPointer<QObject> >();

    if (data->resolved)
        return data->objects;

    data->objects = QpRelationResolver::resolveToManyRelation(data->metaProperty.name(), data->parent);
    data->resolved = true;
    return data->objects;
}

void QpHasManyBase::add(QSharedPointer<QObject> object)
{
    objects(); // resolve

    if (data->objects.contains(object))
        return;

    data->objects.append(object);

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    QSharedPointer<QObject> sharedParent = Qp::sharedFrom(data->parent);
    QString className = data->metaProperty.metaObject().className();

    if (object) {
        if (reverse.isToOneRelationProperty()) {
            reverse.write(object.data(), Qp::Private::variantCast(sharedParent, className));
        }
        else {

            QSharedPointer<QObject> shared = Qp::sharedFrom(data->parent);
            QVariant wrapper = Qp::Private::variantCast(shared, className);

            const QMetaObject *mo = object->metaObject();
            QByteArray methodName = reverse.metaObject().addObjectMethod(reverse).methodSignature();
            int index = mo->indexOfMethod(methodName);

            Q_ASSERT_X(index > 0, Q_FUNC_INFO,
                       QString("You have to add a public slot with the signature '%1' to your '%2' class!")
                       .arg(QString::fromLatin1(methodName))
                       .arg(mo->className())
                       .toLatin1());

            QMetaMethod method = mo->method(index);
            bool result = method.invoke(object.data(), Qt::DirectConnection,
                                        QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data()));
            Q_ASSERT(result);
            Q_UNUSED(result);
        }
    }

    if (!data->objects.contains(object))
        data->objects.append(object);
}

void QpHasManyBase::remove(QSharedPointer<QObject> object)
{
    objects(); // resolve
    int removeCount = data->objects.removeAll(object);
    Q_ASSERT(removeCount <= 1);

    if (removeCount == 0)
        return;

    QpMetaProperty reverse = data->metaProperty.reverseRelation();
    QString className = data->metaProperty.metaObject().className();

    if (object) {
        if (reverse.isToOneRelationProperty()) {
            reverse.write(object.data(), Qp::Private::variantCast(QSharedPointer<QObject>(), className));
        }
        else {
            QSharedPointer<QObject> shared = Qp::sharedFrom(data->parent);
            QVariant wrapper = Qp::Private::variantCast(shared, className);

            const QMetaObject *mo = object->metaObject();
            QByteArray methodName = reverse.metaObject().removeObjectMethod(reverse).methodSignature();
            int index = mo->indexOfMethod(methodName);

            Q_ASSERT_X(index > 0, Q_FUNC_INFO,
                       QString("You have to add a public slot with the signature '%1' to your '%2' class!")
                       .arg(QString::fromLatin1(methodName))
                       .arg(mo->className())
                       .toLatin1());

            QMetaMethod method = mo->method(index);
            bool result = method.invoke(object.data(), Qt::DirectConnection,
                                        QGenericArgument(data->metaProperty.typeName().toLatin1(), wrapper.data()));
            Q_ASSERT(result);
            Q_UNUSED(result);
        }
    }
}

void QpHasManyBase::setObjects(const QList<QSharedPointer<QObject>> objects) const
{
    data->objects = objects;
    data->resolved = true;
}

bool QpHasManyBase::isResolved() const
{
    return data->resolved;
}
