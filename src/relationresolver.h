#ifndef QPERSISTENCE_RELATIONRESOLVER_H
#define QPERSISTENCE_RELATIONRESOLVER_H

#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>

class QpRelationResolver
{
public:
    static QList<QSharedPointer<QObject > > resolveRelation(const QString &name, const QObject *object);
    static QSharedPointer<QObject> resolveToOneRelation(const QString &name, const QObject *object);
    static QList<QSharedPointer<QObject > > resolveToManyRelation(const QString &name, const QObject *object);
};

#endif // QPERSISTENCE_RELATIONRESOLVER_H
