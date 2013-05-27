#ifndef RELATIONRESOLVER_H
#define RELATIONRESOLVER_H

#include <QObject>

#include <QSharedDataPointer>

class QpRelationResolverData;

class QpRelationResolver : public QObject
{
public:
    QpRelationResolver();
    QpRelationResolver(const QpRelationResolver &);
    QpRelationResolver &operator=(const QpRelationResolver &);
    ~QpRelationResolver();

    QSharedPointer<QObject> resolveToOneRelation(const QString &name, const QObject *object);
    QList<QSharedPointer<QObject > > resolveToManyRelation(const QString &name, const QObject *object);

private:
    QSharedDataPointer<QpRelationResolverData> data;
};

#endif // RELATIONRESOLVER_H
