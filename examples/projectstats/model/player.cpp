#include "player.h"

#include "place.h"
#include "livedrink.h"
#include "game.h"
#include "point.h"
#include "round.h"
#include "schmeisserei.h"

#include <QPersistence.h>

namespace NewDatabase {

Player::Player(QObject *parent) :
    QObject(parent),
    m_gender(UnkownGender),
    m_weight(-1),
    m_size(-1),
    m_places("places", this),
    m_liveDrinks("liveDrinks", this),
    m_games("games",this),
    m_points("points",this),
    m_schmeissereien("schmeisereien",this),
    m_hochzeitRounds("hochzeitRounds",this),
    m_trumpfabgabeRounds("trumpfabgabeRounds",this),
    m_soloRounds("soloRounds",this),
    m_schweinereiRounds("schweinereiRounds",this),
    m_re1Rounds("re1Rounds",this),
    m_re2Rounds("re2Rounds",this)
{
}

Player::~Player()
{
}

QPixmap Player::avatar() const
{
    return m_avatar;
}

void Player::setAvatar(const QPixmap &avatar)
{
    m_avatar = avatar;
}

QColor Player::color() const
{
    return m_color;
}

void Player::setColor(const QColor &color)
{
    m_color = color;
}

int Player::size() const
{
    return m_size;
}

void Player::setSize(int size)
{
    m_size = size;
}

int Player::weight() const
{
    return m_weight;
}

void Player::setWeight(int weight)
{
    m_weight = weight;
}

Player::Gender Player::gender() const
{
    return m_gender;
}

void Player::setGender(const Gender &gender)
{
    m_gender = gender;
}

QString Player::name() const
{
    return m_name;
}

void Player::setName(const QString &name)
{
    m_name = name;
}

QList<QSharedPointer<Place> > Player::places() const
{
    return m_places.resolveList();
}

void Player::addPlace(QSharedPointer<Place> place)
{
    m_places.relate(place);
    place->addPlayer(Qp::sharedFrom<Player>(this));
}

void Player::setPlaces(const QList<QSharedPointer<Place> > &places)
{
    m_places.clear();
    m_places.relate(places);
}

QList<QSharedPointer<LiveDrink> > Player::liveDrinks() const
{
    return m_liveDrinks.resolveList();
}

void Player::setLiveDrinks(const QList<QSharedPointer<LiveDrink> > &drinks)
{
    m_liveDrinks.relate(drinks);
}

QList<QSharedPointer<Game> > Player::games() const
{
    return m_games.resolveList();
}

int Player::gamePoints() const
{
    int points = 0;
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    foreach(QSharedPointer<Game> game, games()) {
        points += game->totalPoints(sharedThis);
    }
    return points;
}

int Player::points() const
{
    int points = 0;
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    foreach(QSharedPointer<Game> game, games()) {
        int playerCount = game->players().size();
        int zaehler = playerCount - game->placement(sharedThis);
        int nenner = playerCount - 1;
        points += 100 * zaehler / nenner;
    }
    return points;
}

double Player::average() const
{
    int gameCount = games().size();
    if(gameCount == 0)
        return 0.0;

    return ((double) points()) / gameCount;
}

int Player::wins() const
{
    int wins = 0;
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    foreach(QSharedPointer<Game> game, games()) {
        QList<QSharedPointer<Player> > players = game->playersSortedByPlacement();
        if(players.isEmpty())
            continue;

        if(players.first() == sharedThis)
            ++wins;
    }
    return wins;
}

int Player::losses() const
{
    int losses = 0;
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    foreach(QSharedPointer<Game> game, games()) {
        QList<QSharedPointer<Player> > players = game->playersSortedByPlacement();
        if(players.isEmpty())
            continue;

        if(players.last() == sharedThis)
            ++losses;
    }
    return losses;
}

double Player::averagePlacement() const
{
    int gameCount = games().size();
    if(gameCount == 0)
        return 0.0;

    double result = 0;
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    foreach(QSharedPointer<Game> game, games()) {
        result += game->placement(sharedThis);
    }
    return result / gameCount;
}

bool sortGamesByDate(const QSharedPointer<Game> &g1, const QSharedPointer<Game> &g2) {
    return g1->creationTime() < g2->creationTime();
}

QSharedPointer<Game> Player::lastGame() const
{
    QList<QSharedPointer<Game> > gs = games();
    if(gs.isEmpty())
        return QSharedPointer<Game>();

    qSort(gs.begin(), gs.end(), &sortGamesByDate);

    return gs.first();
}

QSharedPointer<Game> Player::lastWin() const
{
    QList<QSharedPointer<Game> > gs = games();
    if(gs.isEmpty())
        return QSharedPointer<Game>();

    qSort(gs.begin(), gs.end(), &sortGamesByDate);

    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    foreach(QSharedPointer<Game> game, gs) {
        if(game->placement(sharedThis) == 1)
            return game;
    }

    return QSharedPointer<Game>();
}

void Player::setGames(const QList<QSharedPointer<Game> > &games)
{
    m_games.clear();
    m_games.relate(games);
}

QList<QSharedPointer<Point> > Player::pointsInstances() const
{
    return m_points.resolveList();
}

void Player::setPointInstances(const QList<QSharedPointer<Point> > &pointInstances)
{
    m_points.clear();
    m_points.relate(pointInstances);
}

QList<QSharedPointer<Round> > Player::re2Rounds() const
{
    return m_re2Rounds.resolveList();
}

QList<QSharedPointer<Round> > Player::reRounds() const
{
    QList<QSharedPointer<Round> > result = re1Rounds();
    result.append(re2Rounds());
    return result;
}

double Player::rePercentage() const
{
    int roundCount = rounds().size();
    if(roundCount == 0)
        return -1.0;

    return reRounds().size() / roundCount;
}

QList<QSharedPointer<Round> > Player::reWins() const
{
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, reRounds()) {
        if(round->winnerParty() == Round::Re) {
            result.append(round);
        }
    }
    return result;
}

double Player::reWinsPercentage() const
{
    int roundCount = rounds().size();
    if(roundCount == 0)
        return -1.0;

    return reWins().size() / roundCount;
}

QList<QSharedPointer<Round> > Player::contraRounds() const
{
    QList<QSharedPointer<Round> > result;
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    foreach(QSharedPointer<Round> round, rounds()) {
        if(round->contraPlayers().contains(sharedThis)) {
            result.append(round);
        }
    }
    return result;
}

double Player::contraPercentage() const
{
    int roundCount = rounds().size();
    if(roundCount == 0)
        return -1.0;

    return contraRounds().size() / roundCount;
}

QList<QSharedPointer<Round> > Player::contraWins() const
{
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, contraRounds()) {
        if(round->winnerParty() == Round::Contra) {
            result.append(round);
        }
    }
    return result;
}

double Player::contraWinsPercentage() const
{
    int roundCount = rounds().size();
    if(roundCount == 0)
        return -1.0;

    return contraWins().size() / roundCount;
}

QList<QSharedPointer<Round> > Player::hochzeitenRounds() const
{
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, re1Rounds()) {
        if(round->hochzeitPlayer()) {
            Q_ASSERT(round->hochzeitPlayer().data() == this);
            result.append(round);
        }
    }
    return result;
}

QList<QSharedPointer<Round> > Player::soliRounds() const
{
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, re1Rounds()) {
        if(round->soloPlayer()) {
            Q_ASSERT(round->soloPlayer().data() == this);
            result.append(round);
        }
    }
    return result;
}

QList<QSharedPointer<Round> > Player::trumpfabgabenRounds() const
{
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, reRounds()) {
        if(round->trumpfabgabePlayer()) {
            result.append(round);
        }
    }
    return result;
}

QList<QSharedPointer<Round> > Player::trumpfabgabenAbgegebenRounds() const
{
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, reRounds()) {
        if(round->trumpfabgabePlayer() == sharedThis) {
            result.append(round);
        }
    }
    return result;
}

QList<QSharedPointer<Round> > Player::trumpfabgabenAufgenommenRounds() const
{
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, reRounds()) {
        QSharedPointer<Player> trumpfabgabe = round->trumpfabgabePlayer();
        if(trumpfabgabe && trumpfabgabe != sharedThis) {
            result.append(round);
        }
    }
    return result;
}

QList<QSharedPointer<Round> > Player::schweinereienRounds() const
{
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Round> round, reRounds()) {
        if(round->schweinereiPlayer() == sharedThis) {
            result.append(round);
        }
    }
    return result;
}

QList<QSharedPointer<Round> > Player::schmeissereienRounds() const
{
    QList<QSharedPointer<Round> > result;
    foreach(QSharedPointer<Schmeisserei> s, schmeissereien()) {
        result.append(s->round());
    }
    return result;
}

double Player::averagePointsPerRound() const
{
    int roundCount = rounds().size();
    if(roundCount == 0)
        return 0.0;

    return gamePoints() / roundCount;
}

QList<QSharedPointer<Round> > Player::winRounds() const
{
    QList<QSharedPointer<Round> > result = reWins();
    result.append(contraWins());
    return result;
}

double Player::roundWinsPercentage() const
{
    int roundCount = rounds().size();
    if(roundCount == 0)
        return 0.0;

    return winRounds().size() / roundCount;
}

void Player::setRe2Rounds(const QList<QSharedPointer<Round> > &re2Rounds)
{
    m_re2Rounds.relate(re2Rounds);
}

QList<QSharedPointer<Round> > Player::re1Rounds() const
{
    return m_re1Rounds.resolveList();
}

void Player::setRe1Rounds(const QList<QSharedPointer<Round> > &re1Rounds)
{
    m_re1Rounds.relate(re1Rounds);
}

QList<QSharedPointer<Round> > Player::schweinereiRounds() const
{
    return m_schweinereiRounds.resolveList();
}

void Player::setSchweinereiRounds(const QList<QSharedPointer<Round> > &schweinereiRounds)
{
    m_schweinereiRounds.relate(schweinereiRounds);
}

QList<QSharedPointer<Round> > Player::soloRounds() const
{
    return m_soloRounds.resolveList();
}

void Player::setSoloRounds(const QList<QSharedPointer<Round> > &soloRounds)
{
    m_soloRounds.relate(soloRounds);
}

QList<QSharedPointer<Round> > Player::trumpfabgabeRounds() const
{
    return m_trumpfabgabeRounds.resolveList();
}

void Player::setTrumpfabgabeRounds(const QList<QSharedPointer<Round> > &trumpfabgabeRounds)
{
    m_trumpfabgabeRounds.relate(trumpfabgabeRounds);
}

QList<QSharedPointer<Round> > Player::hochzeitRounds() const
{
    return m_hochzeitRounds.resolveList();
}

void Player::setHochzeitRounds(const QList<QSharedPointer<Round> > &hochzeitRounds)
{
    m_hochzeitRounds.relate(hochzeitRounds);
}

QList<QSharedPointer<Schmeisserei> > Player::schmeissereien() const
{
    return m_schmeissereien.resolveList();
}

QList<QSharedPointer<Round> > Player::rounds() const
{
    QList<QSharedPointer<Round> > result;
    QSharedPointer<Player> sharedThis = Qp::sharedFrom<Player>(this);

    foreach(QSharedPointer<Game> game, games()) {
        foreach(QSharedPointer<Round> round, game->rounds()) {
            if(round->playingPlayers().contains(sharedThis))
                result.append(round);
        }
    }
    return result;
}

void Player::setSchmeissereien(const QList<QSharedPointer<Schmeisserei> > &schmeissereien)
{
    m_schmeissereien.relate(schmeissereien);
}

} // namespace NewDatabase
