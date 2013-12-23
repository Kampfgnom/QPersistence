#ifndef RELATIONRESOLVER_H
#define RELATIONRESOLVER_H

#include <QObject>

#include <QSharedDataPointer>

class QpRelationResolverData;

class QpRelationResolver : public QObject
{
public:
    static QList<QSharedPointer<QObject > > resolveRelation(const QString &name, const QObject *object);
    static QSharedPointer<QObject> resolveToOneRelation(const QString &name, const QObject *object);
    static QList<QSharedPointer<QObject > > resolveToManyRelation(const QString &name, const QObject *object);
};

#endif // RELATIONRESOLVER_H
