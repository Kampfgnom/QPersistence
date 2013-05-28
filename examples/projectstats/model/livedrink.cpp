#include "livedrink.h"

#include "player.h"
#include "round.h"
#include "drink.h"

namespace NewDatabase {

LiveDrink::LiveDrink(QObject *parent) :
    QObject(parent),
    m_time(QDateTime::currentDateTime()),
    m_player("player",this),
    m_round("round",this),
    m_drink("drink",this)
{
}

LiveDrink::~LiveDrink()
{
}

QSharedPointer<Player> LiveDrink::player() const
{
    return m_player.resolve();
}

void LiveDrink::setPlayer(QSharedPointer<Player> player)
{
    m_player.relate(player);
}

QSharedPointer<Round> LiveDrink::round() const
{
    return m_round.resolve();
}

void LiveDrink::setRound(QSharedPointer<Round> round)
{
    m_round.relate(round);
}

QSharedPointer<Drink> LiveDrink::drink() const
{
    return m_drink.resolve();
}

void LiveDrink::setDrink(QSharedPointer<Drink> drink)
{
    m_drink.relate(drink);
}

QDateTime LiveDrink::time() const
{
    return m_time;
}

void LiveDrink::setTime(const QDateTime &time)
{
    m_time = time;
}

} // namespace NewDatabase
