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

QSharedPointer<QObject> QpRelationResolver::resolveToOneRelation(const QString &name, const QObject *object)
{
    QpMetaObject metaObject = Qp::Private::metaObject(object->metaObject()->className());
    QpMetaProperty relation = metaObject.metaProperty(name);
    const char* column = relation.columnName().toLatin1();

    int foreignKey = object->property(column).toInt();

    if(foreignKey == 0) {
        QpSqlDataAccessObjectHelper *helper = QpSqlDataAccessObjectHelper::forDatabase(Qp::database());
        foreignKey = helper->foreignKey(relation, const_cast<QObject*>(object));
    }

    if(foreignKey == 0)
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
        Q_ASSERT(relatedObject);
        relatedObjects.append(relatedObject);
    }

    return relatedObjects;
}
