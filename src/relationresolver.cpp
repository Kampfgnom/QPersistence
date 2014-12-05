#include "relationresolver.h"

#include "metaobject.h"
#include "metaproperty.h"
#include "private.h"
#include "qpersistence.h"
#include "sqldataaccessobjecthelper.h"
#include "conversion.h"
#include "storage.h"

void QpRelationResolver::readRelationFromDatabase(const QpMetaProperty &relation, QObject *object)
{
    QList<QSharedPointer<QObject>> relatedObjects = resolveRelation(relation.name(), object);

    QVariant variant;
    QpMetaProperty::Cardinality cardinality = relation.cardinality();
    if (cardinality == QpMetaProperty::OneToManyCardinality
            || cardinality == QpMetaProperty::ManyToManyCardinality) {
        variant = Qp::Private::variantListCast(relatedObjects, relation.reverseMetaObject().className());
    }
    else {
        if(relatedObjects.isEmpty()) {
            variant = Qp::Private::variantCast(QSharedPointer<QObject>(), relation.reverseMetaObject().className());
        }
        else {
            Q_ASSERT(relatedObjects.size() == 1);
            variant = Qp::Private::variantCast(relatedObjects.first(), relation.reverseMetaObject().className());
        }
    }

    relation.metaProperty().write(object, variant);
}

QList<QSharedPointer<QObject> > QpRelationResolver::resolveRelation(const QString &name, const QObject *object)
{
    QList<QSharedPointer<QObject> > result;

    QpMetaObject metaObject = QpMetaObject::forObject(object);
    QpMetaProperty relation = metaObject.metaProperty(name);

    QpMetaProperty::Cardinality cardinality = relation.cardinality();

    if (cardinality == QpMetaProperty::OneToManyCardinality
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
    QpMetaObject metaObject = QpMetaObject::forObject(object);
    QpMetaProperty relation = metaObject.metaProperty(name);
    QByteArray column;
    if(relation.hasTableForeignKey()) {
       column = relation.columnName().toLatin1();
    }
    else {
        column = QByteArray("_Qp_FK_") + relation.name().toLatin1();
    }

    QpStorage *storage = QpStorage::forObject(object);
    QVariant variantForeignKey = object->property(column);
    int foreignKey = variantForeignKey.toInt();

    if (!variantForeignKey.isValid()) {
        QpSqlDataAccessObjectHelper *helper = storage->sqlDataAccessObjectHelper();
        foreignKey = helper->foreignKey(relation, const_cast<QObject*>(object));
    }

    if (foreignKey <= 0)
        return QSharedPointer<QObject>();

    QpMetaObject foreignMetaObject = relation.reverseMetaObject();
    QSharedPointer<QObject> related = storage->dataAccessObject(foreignMetaObject.metaObject())->readObject(foreignKey);
    if(Qp::Private::isDeleted(related.data()))
        return QSharedPointer<QObject>();

    if (!related)
        const_cast<QObject*>(object)->setProperty(column, 0);

    return related;
}

QList<QSharedPointer<QObject> > QpRelationResolver::resolveToManyRelation(const QString &name, const QObject *object)
{
    QpMetaObject metaObject = QpMetaObject::forObject(object);
    QpMetaProperty relation = metaObject.metaProperty(name);

    QpStorage *storage = QpStorage::forObject(object);
    QpSqlDataAccessObjectHelper *helper = storage->sqlDataAccessObjectHelper();
    QList<int> foreignKeys = helper->foreignKeys(relation, const_cast<QObject*>(object));


    QpMetaObject foreignMetaObject = relation.reverseMetaObject();
    QpDaoBase *dao = storage->dataAccessObject(foreignMetaObject.metaObject());

    QList<QSharedPointer<QObject> > relatedObjects;
    relatedObjects.reserve(foreignKeys.size());
    foreach (int key, foreignKeys) {
        if(key <= 0) {
            qWarning() << QString("The object of type '%1' with the ID '%2' is related to a NULL object")
                          .arg(metaObject.className())
                          .arg(Qp::Private::primaryKey(object));
            Q_ASSERT_X(false, Q_FUNC_INFO, "invalid relation in db");
            continue;
        }

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

        // here we explicitly check for deleted objects, because for many-to-many relations,
        // we cannot use the mysql server for filtering the related objects
        if(Qp::Private::isDeleted(relatedObject.data()))
            continue;

        relatedObjects.append(relatedObject);
    }

    return relatedObjects;
}
