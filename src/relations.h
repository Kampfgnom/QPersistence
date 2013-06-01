#ifndef RELATIONS_H
#define RELATIONS_H

#include <QSharedDataPointer>
#include <QSharedPointer>

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
    class QpWeakRelationData : public QSharedData {
    public:
        QString name;
        QObject *parent;
        mutable bool resolved;
        mutable QList<QWeakPointer<T> > relatedList;
        mutable QWeakPointer<T> related;
    };

    QList<QSharedPointer<T> > resolveFromDatabase() const;

    QSharedDataPointer<QpWeakRelationData> data;
};

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
    class QpStrongRelationData : public QSharedData {
    public:
        QString name;
        QObject *parent;
        mutable bool resolved;
        mutable QList<QSharedPointer<T> > relatedList;
        mutable QSharedPointer<T> related;
    };

    void resolveFromDatabase() const;

    QSharedDataPointer<QpStrongRelationData> data;
};

#include "relations.cpp"

#endif // RELATIONS_H
