#ifndef QPERSISTENCE_RELATIONRESOLVER_H
#define QPERSISTENCE_RELATIONRESOLVER_H

#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>

class QpMetaProperty;

class QpRelationResolver
{
public:
    static void readRelationFromDatabase(const QpMetaProperty &relation, QObject *object);
    static QList<QSharedPointer<QObject > > resolveRelation(const QString &name, const QObject *object);
    static QSharedPointer<QObject> resolveToOneRelation(const QString &name, const QObject *object);
    static QList<QSharedPointer<QObject > > resolveToManyRelation(const QString &name, const QObject *object);
};

#endif // QPERSISTENCE_RELATIONRESOLVER_H
