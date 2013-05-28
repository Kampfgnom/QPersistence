#include "schmeisserei.h"

#include "player.h"
#include "round.h"

namespace NewDatabase {

Schmeisserei::Schmeisserei(QObject *parent) :
    QObject(parent),
    m_type(UnkownType),
    m_round("round",this),
    m_player("player",this)
{
}

QSharedPointer<Player> Schmeisserei::player() const
{
    return m_player.resolve();
}

void Schmeisserei::setPlayer(const QSharedPointer<Player> &player)
{
    m_player.relate(player);
}

QSharedPointer<Round> Schmeisserei::round() const
{
    return m_round.resolve();
}

void Schmeisserei::setRound(const QSharedPointer<Round> &round)
{
    m_round.relate(round);
}

QString Schmeisserei::typeString() const
{
    return m_typeString;
}

void Schmeisserei::setTypeString(const QString &typeString)
{
    m_typeString = typeString;
}

Schmeisserei::Type Schmeisserei::type() const
{
    return m_type;
}

void Schmeisserei::setType(const Type &type)
{
    m_type = type;
}

} // namespace NewDatabase
