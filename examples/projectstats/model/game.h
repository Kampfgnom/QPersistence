#ifndef GAME_H
#define GAME_H

#include <QObject>

#include <QPersistenceRelations.h>
#include <QTime>
#include <QDateTime>
#include <QTimer>

namespace NewDatabase {

class Round;
class Place;
class Player;
class LiveDrink;

class Game: public QObject
{
    Q_OBJECT
    Q_ENUMS(Type)
    Q_ENUMS(State)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(Type type READ type WRITE setType)
    Q_PROPERTY(QDateTime creationTime READ creationTime WRITE setCreationTime)
    Q_PROPERTY(QString comment READ comment WRITE setComment)
    Q_PROPERTY(State state READ state WRITE setState)
    Q_PROPERTY(QTime length READ length WRITE setLength)
    Q_PROPERTY(QSharedPointer<Place> site READ site WRITE setSite)
    Q_PROPERTY(QList<QSharedPointer<Player> > players READ players WRITE setPlayers)
    Q_PROPERTY(QList<QSharedPointer<Round> > rounds READ rounds WRITE setRounds)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:site",
                "reverserelation=games")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:players",
                "reverserelation=games")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:rounds",
                "reverserelation=game")

public:
    enum Type {
        UnkownType,
        Doppelkopf
    };

    enum State {
        Running,
        Paused,
        Finished
    };

    explicit Game(QObject *parent = 0);
    ~Game();

    QString name() const;
    void setName(const QString &name);

    Type type() const;
    void setType(const Type &type);

    QDateTime creationTime() const;

    QString comment() const;
    void setComment(const QString &comment);

    State state() const;
    void setState(State state);

    QTime length() const;

    QSharedPointer<Place> site() const;

    QList<QSharedPointer<Player> > players() const;
    QSharedPointer<Player> currentCardMixer() const;
    QList<QSharedPointer<Player> > currentPlayingPlayers() const;

    QList<QSharedPointer<Round> > rounds() const;
    QSharedPointer<Round> currentRound() const;

    QList<QSharedPointer<Player> > playersSortedByPlacement() const;
    int placement(QSharedPointer<Player> player) const;
    int placementAfterRounds(QSharedPointer<Player> player, int roundNumber) const;
    double averagePlacement(QSharedPointer<Player> player) const;
    int leadingRoundCount(QSharedPointer<Player> player) const;
    int totalPoints() const;
    int totalPoints(QSharedPointer<Player> player) const;

    QList<QSharedPointer<LiveDrink> > drinks() const;
    QList<QSharedPointer<LiveDrink> > drinks(QSharedPointer<Player> player) const;

    double completedPercentage() const;
    bool isComplete() const;

    bool hasPflichtSolo(QSharedPointer<Player> player) const;

    int hochzeitCountAfterRounds(int roundCount);
    int soloCountAfterRounds(int roundCount);
    int pflichtSoloCountAfterRounds(int roundCount);
    int trumpfabgabeCountAfterRounds(int roundCount);
    int schweinereiCountAfterRounds(int roundCount);
    int schmeissereiCountAfterRounds(int roundCount);

private:
    void setCreationTime(const QDateTime &creationTime);
    void setLength(const QTime &length);
    void setSite(QSharedPointer<Place> site);
    void setPlayers(const QList<QSharedPointer<Player> > &players);
    void setRounds(const QList<QSharedPointer<Round> > &rounds);

    QString m_name;
    Type m_type;
    QDateTime m_creationTime;
    QString m_comment;
    State m_state;
    QTime m_length;

    QpWeakRelation<Place> m_site;
    QpWeakRelation<Player> m_players;
    QpStrongRelation<Round> m_rounds;

    QTimer m_lengthTimer;
};

} // namespace NewDatabase

#endif // GAME_H
