#include "relationresolver.h"
#include <QSharedData>

#include "metaobject.h"
#include "metaproperty.h"
#include "sqldataaccessobjecthelper.h"
#include "private.h"
#include "qpersistence.h"

class QpRelationResolverData : public QSharedData {
public:
};

QpRelationResolver::QpRelationResolver() : data(new QpRelationResolverData)
{
}

QpRelationResolver::QpRelationResolver(const QpRelationResolver &rhs) : data(rhs.data)
{
}

QpRelationResolver &QpRelationResolver::operator=(const QpRelationResolver &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QpRelationResolver::~QpRelationResolver()
{
}

QList<QSharedPointer<QObject> > QpRelationResolver::resolveRelation(const QString &name, const QObject *object)
{
    QList<QSharedPointer<QObject> > result;

    QpMetaObject metaObject = Qp::Private::metaObject(object->metaObject()->className());
    QpMetaProperty relation = metaObject.metaProperty(name);

    QpMetaProperty::Cardinality cardinality = relation.cardinality();

    if(cardinality == QpMetaProperty::ToManyCardinality
            || cardinality == QpMetaProperty::OneToManyCardinality
            || cardinality == QpMetaProperty::ManyToManyCardinality) {
        result = resolveToManyRelation(name, object);
    }
    else {
        result.append(resolveToOneRelation(name, object));
    }

    return result;
}

QSharedPointer<QObject> QpRelationResolver::resolveToOneRelation(const QString &name, const QObject *object)
{
    QpMetaObject metaObject = Qp::Private::metaObject(object->metaObject()->className());
    QpMetaProperty relation = metaObject.metaProperty(name);
    const char* column = relation.columnName().toLatin1();

    QVariant variantForeignKey = object->property(column);
    int foreignKey = variantForeignKey.toInt();

    if(!variantForeignKey.isValid()) {
        QpSqlDataAccessObjectHelper *helper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
        foreignKey = helper->foreignKey(relation, const_cast<QObject*>(object));
    }

    if(foreignKey <= 0)
        return QSharedPointer<QObject>();

    QpMetaObject foreignMetaObject = relation.reverseMetaObject();
    QSharedPointer<QObject> related = Qp::Private::dataAccessObject(foreignMetaObject)->readObject(foreignKey);

    if(!related)
        const_cast<QObject*>(object)->setProperty(column, 0);

    return related;
}

QList<QSharedPointer<QObject> > QpRelationResolver::resolveToManyRelation(const QString &name, const QObject *object)
{
    QpMetaObject metaObject = Qp::Private::metaObject(object->metaObject()->className());
    QpMetaProperty relation = metaObject.metaProperty(name);

    QpSqlDataAccessObjectHelper *helper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
    QList<int> foreignKeys = helper->foreignKeys(relation, const_cast<QObject*>(object));


    QpMetaObject foreignMetaObject = relation.reverseMetaObject();
    QpDaoBase *dao = Qp::Private::dataAccessObject(foreignMetaObject);

    QList<QSharedPointer<QObject> > relatedObjects;
    relatedObjects.reserve(foreignKeys.size());
    foreach(int key, foreignKeys) {
        QSharedPointer<QObject> relatedObject = dao->readObject(key);
        Q_ASSERT_X(relatedObject,
                   Q_FUNC_INFO,
                   QString("It appears, that there is no '%1' object with the ID '%2',"
                           "although the '%3' relation of the '%4' class refers to this ID.")
                   .arg(foreignMetaObject.className())
                   .arg(key)
                   .arg(relation.name())
                   .arg(metaObject.className())
                   .toLatin1());
        relatedObjects.append(relatedObject);
    }

    return relatedObjects;
}
