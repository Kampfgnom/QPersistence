#ifndef RELATIONS_H
#define RELATIONS_H

#include <QSharedDataPointer>
#include <QSharedPointer>


template<class T>
class QpWeakRelationData;

template<class T>
class QpWeakRelation
{
public:
    QpWeakRelation(const QString &name, QObject *parent);
    QpWeakRelation(const QpWeakRelation &);
    QpWeakRelation &operator=(const QpWeakRelation &);
    ~QpWeakRelation();
    
    QList<QSharedPointer<T> > resolveList() const;
    QSharedPointer<T> resolve() const;

    void relate(QSharedPointer<T> related);
    void relate(QList<QSharedPointer<T> > related);
    void unrelate(QSharedPointer<T> related);
    void clear();

private:
    QList<QSharedPointer<T> > resolveFromDatabase() const;

    QSharedDataPointer<QpWeakRelationData<T> > data;
};

template<class T>
class QpStrongRelationData;

template<class T>
class QpStrongRelation
{
public:
    QpStrongRelation(const QString &name, QObject *parent);
    QpStrongRelation(const QpStrongRelation &);
    QpStrongRelation &operator=(const QpStrongRelation &);
    ~QpStrongRelation();

    QList<QSharedPointer<T> > resolveList() const;
    QSharedPointer<T> resolve() const;

    void relate(QSharedPointer<T> related);
    void relate(QList<QSharedPointer<T> > related);
    void unrelate(QSharedPointer<T> related);
    void clear();

private:
    void resolveFromDatabase() const;

    QSharedDataPointer<QpStrongRelationData<T> > data;
};

#include "relations.cpp"

#endif // RELATIONS_H
