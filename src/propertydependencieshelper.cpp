#include "propertydependencieshelper.h"

#include "metaproperty.h"
#include "storage.h"

#include <QSharedData>

// Need qHash for QSet.
// This hash implementation is closely tied to QpPropertyDependencies' specific use-case
inline uint qHash(const QMetaMethod &t, uint seed) {
    // Only hash method's name, because we may assume,
    // that all methods per QSet are of the same class
    return qHash(t.name(), seed);
}

class QpPropertyDependenciesData : public QSharedData
{
public:
    enum ChangeAction {
        ConnectAction,
        DisconnectAction
    };

    QpStorage *storage;

    QSet<QMetaMethod> initSelfDependencies(QSharedPointer<QObject> object);
    QSet<QMetaMethod> changeRelationDependencies(QObject *object,
                                                 QSharedPointer<QObject> related,
                                                 const QpMetaProperty &relation,
                                                 ChangeAction changeAction);
    QSet<QMetaMethod> changeDependencies(QObject *object,
                                         QSharedPointer<QObject> other,
                                         const QpMetaProperty &property,
                                         const QString &relationName,
                                         ChangeAction changeAction);
    QMetaMethod changeDependency(QObject *object,
                                 QSharedPointer<QObject> other,
                                 const QpMetaProperty &property,
                                 const QString &dependency,
                                 const QString &relationName,
                                 ChangeAction changeAction);
    QMetaMethod changeDependency(QObject *object,
                                 QSharedPointer<QObject> other,
                                 const QpMetaProperty &property,
                                 const QString &dependencyPropertyName,
                                 ChangeAction changeAction);
};

QSet<QMetaMethod> QpPropertyDependenciesData::initSelfDependencies(QSharedPointer<QObject> object)
{
    QSet<QMetaMethod> result;
    QpMetaObject metaObject = QpMetaObject::forObject(object);
    foreach (QpMetaProperty property, metaObject.calculatedProperties()) {
        result.unite(changeDependencies(object.data(), object, property, QString::fromLatin1("this"), ConnectAction));
    }
    return result;
}

QSet<QMetaMethod> QpPropertyDependenciesData::changeRelationDependencies(QObject *object,
                                                                         QSharedPointer<QObject> related,
                                                                         const QpMetaProperty &relation,
                                                                         ChangeAction changeAction)
{
    QSet<QMetaMethod> result;
    QString relationName = relation.name();
    QpMetaObject metaObject = QpMetaObject::forObject(object);
    foreach (QpMetaProperty property, metaObject.calculatedProperties()) {
        result.unite(changeDependencies(object, related, property, relationName, changeAction));
    }
    return result;
}

QSet<QMetaMethod> QpPropertyDependenciesData::changeDependencies(QObject *object,
                                                                 QSharedPointer<QObject> other,
                                                                 const QpMetaProperty &property,
                                                                 const QString &relationName,
                                                                 ChangeAction changeAction)
{
    QSet<QMetaMethod> result;
    foreach (QString dependecy, property.dependencies()) {
        QMetaMethod recalculateMethod = changeDependency(object, other, property, dependecy, relationName, changeAction);
        if (recalculateMethod.isValid())
            result.insert(recalculateMethod);
    }
    return result;
}

QMetaMethod QpPropertyDependenciesData::changeDependency(QObject *object,
                                                         QSharedPointer<QObject> other,
                                                         const QpMetaProperty &property,
                                                         const QString &dependency,
                                                         const QString &relationName,
                                                         ChangeAction changeAction)
{
    QStringList dependencySplit = dependency.split('.');
    Q_ASSERT(dependencySplit.size() == 2);
    QString dependencyRelation = dependencySplit.first();
    QString dependencySignalName = dependencySplit.last();

    if (dependencyRelation != relationName) {
        return QMetaMethod();
    }

    return changeDependency(object, other, property, dependencySignalName, changeAction);
}

QMetaMethod QpPropertyDependenciesData::changeDependency(QObject *object,
                                                         QSharedPointer<QObject> other,
                                                         const QpMetaProperty &property,
                                                         const QString &dependencySignalName,
                                                         ChangeAction changeAction)
{
    QString methodName = property.name();
    methodName[0] = methodName.at(0).toTitleCase();
    methodName.prepend("recalculate").append("()");

    QByteArray signature = QMetaObject::normalizedSignature(methodName.toLatin1());
    int index = object->metaObject()->indexOfMethod(signature);
    Q_ASSERT_X(index >= 0,
               Q_FUNC_INFO,
               QString::fromLatin1("No such method '%1::%2'")
               .arg(object->metaObject()->className())
               .arg(QString::fromLatin1(signature))
               .toLatin1());
    QMetaMethod recalculateMethod = object->metaObject()->method(index);

    methodName = dependencySignalName;
    signature = QMetaObject::normalizedSignature(methodName.toLatin1());
    index = other->metaObject()->indexOfSignal(signature);
    Q_ASSERT_X(index >= 0,
               Q_FUNC_INFO,
               QString::fromLatin1("No such signal '%1::%2'")
               .arg(other->metaObject()->className())
               .arg(QString::fromLatin1(signature))
               .toLatin1());
    QMetaMethod changeSignal = other->metaObject()->method(index);

    switch (changeAction) {
    case QpPropertyDependenciesData::ConnectAction:
        object->connect(other.data(), changeSignal, object, recalculateMethod,
                        static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection));
        break;
    case QpPropertyDependenciesData::DisconnectAction:
        object->disconnect(other.data(), changeSignal, object, recalculateMethod);
        break;
    }

    return recalculateMethod;
}

QpPropertyDependenciesHelper::QpPropertyDependenciesHelper(QpStorage *storage) :
    data(new QpPropertyDependenciesData)
{
    data->storage = storage;
}

QpPropertyDependenciesHelper::QpPropertyDependenciesHelper(const QpPropertyDependenciesHelper &other) :
    data(other.data)
{
}

QpPropertyDependenciesHelper &QpPropertyDependenciesHelper::operator =(const QpPropertyDependenciesHelper &other)
{
    if (this != &other) {
        data.operator =(other.data);
    }

    return *this;
}

QpPropertyDependenciesHelper::~QpPropertyDependenciesHelper()
{
}

void QpPropertyDependenciesHelper::initSelfDependencies(QSharedPointer<QObject> object) const
{
    Q_ASSERT(object);
    if (!object)
        return;

    data->initSelfDependencies(object);
}

void QpPropertyDependenciesHelper::initDependencies(QObject *object,
                                              QList<QSharedPointer<QObject> > relatedObjects,
                                              const QpMetaProperty &relation) const
{
    Q_ASSERT(object);
    QSet<QMetaMethod> recalculateMethods;
    foreach (QSharedPointer<QObject> related, relatedObjects) {
        recalculateMethods.unite(data->changeRelationDependencies(object,
                                                                  related,
                                                                  relation,
                                                                  QpPropertyDependenciesData::ConnectAction));
    }

    foreach (QMetaMethod method, recalculateMethods) {
        method.invoke(object);
    }
}

void QpPropertyDependenciesHelper::initDependencies(QObject *object,
                                              QSharedPointer<QObject> related,
                                              const QpMetaProperty &relation) const
{
    Q_ASSERT(object);
    if (!related)
        return;

    QSet<QMetaMethod> recalculateMethods = data->changeRelationDependencies(object,
                                                                            related,
                                                                            relation,
                                                                            QpPropertyDependenciesData::ConnectAction);
    foreach (QMetaMethod method, recalculateMethods) {
        method.invoke(object);
    }
}

void QpPropertyDependenciesHelper::removeDependencies(QObject *object,
                                                QSharedPointer<QObject> related,
                                                const QpMetaProperty &relation) const
{
    Q_ASSERT(object);
    if (!related)
        return;
    QSet<QMetaMethod> recalculateMethods = data->changeRelationDependencies(object,
                                                                            related,
                                                                            relation,
                                                                            QpPropertyDependenciesData::DisconnectAction);
    foreach (QMetaMethod method, recalculateMethods) {
        method.invoke(object);
    }
}
