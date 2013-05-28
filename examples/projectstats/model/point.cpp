#include "point.h"

#include "round.h"
#include "player.h"

namespace NewDatabase {

Point::Point(QObject *parent) :
    QObject(parent),
    m_points(0),
    m_player("player",this),
    m_round("round",this)
{
}

int Point::points() const
{
    return m_points;
}

void Point::setPoints(int points)
{
    m_points = points;
}

QSharedPointer<Player> Point::player() const
{
    return m_player.resolve();
}

void Point::setPlayer(const QSharedPointer<Player> &player)
{
    m_player.relate(player);
}

QSharedPointer<Round> Point::round() const
{
    return m_round.resolve();
}

void Point::setRound(const QSharedPointer<Round> &round)
{
    m_round.relate(round);
}

} // namespace NewDatabase
