#ifndef POINT_H
#define POINT_H

#include <QObject>

#include <QPersistenceRelations.h>

namespace NewDatabase {

class Player;
class Round;

class Point : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int points READ points WRITE setPoints)
    Q_PROPERTY(QSharedPointer<Round> round READ round WRITE setRound)
    Q_PROPERTY(QSharedPointer<Player> player READ player WRITE setPlayer)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:round",
                "reverserelation=pointInstances")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:player",
                "reverserelation=pointInstances")

public:
    explicit Point(QObject *parent = 0);

    int points() const;
    void setPoints(int points);

    QSharedPointer<Round> round() const;
    void setRound(const QSharedPointer<Round> &round);

    QSharedPointer<Player> player() const;
    void setPlayer(const QSharedPointer<Player> &player);

private:
    int m_points;
    QpWeakRelation<Player> m_player;
    QpWeakRelation<Round> m_round;
    
};

} // namespace NewDatabase

#endif // POINT_H
