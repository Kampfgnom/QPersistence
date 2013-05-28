#ifndef ROUND_H
#define ROUND_H

#include <QObject>

#include <QPersistenceRelations.h>
#include <QDateTime>
#include <QDate>
#define COMMA ,

namespace NewDatabase {

class Game;
class LiveDrink;
class Player;
class Point;
class Schmeisserei;

class Round : public QObject
{
    Q_OBJECT
    Q_ENUMS(WinnerParty)
    Q_ENUMS(SoloType)
    Q_PROPERTY(int number READ number WRITE setNumber)
    Q_PROPERTY(QDateTime startTime READ startTime WRITE setStartTime)
    Q_PROPERTY(QTime length READ length WRITE setLength)
    Q_PROPERTY(QString comment READ comment WRITE setComment)
    Q_PROPERTY(QString soloTypeString READ soloTypeString WRITE setSoloTypeString)
    Q_PROPERTY(SoloType soloType READ soloType WRITE setSoloType)
    Q_PROPERTY(bool isPflicht READ isPflicht WRITE setIsPflicht)
    Q_PROPERTY(WinnerParty winnerParty READ winnerParty WRITE setWinnerParty)

    Q_PROPERTY(QMap<int COMMA int> points READ _points WRITE setPoints)

    Q_PROPERTY(QSharedPointer<Game> game READ game WRITE setGame)
    Q_PROPERTY(QList<QSharedPointer<LiveDrink> > drinks READ drinks WRITE setDrinks)
//    Q_PROPERTY(QList<QSharedPointer<Point> > pointInstances READ pointInstances WRITE setPointInstances)
    Q_PROPERTY(QList<QSharedPointer<Schmeisserei> > schmeissereien READ schmeissereien WRITE setSchmeissereien)

    Q_PROPERTY(QSharedPointer<Player> hochzeitPlayer READ hochzeitPlayer WRITE setHochzeitPlayer)
    Q_PROPERTY(QSharedPointer<Player> trumpfabgabePlayer READ trumpfabgabePlayer WRITE setTrumpfabgabePlayer)
    Q_PROPERTY(QSharedPointer<Player> soloPlayer READ soloPlayer WRITE setSoloPlayer)
    Q_PROPERTY(QSharedPointer<Player> schweinereiPlayer READ schweinereiPlayer WRITE setSchweinereiPlayer)
    Q_PROPERTY(QSharedPointer<Player> re1Player READ re1Player WRITE setRe1Player)
    Q_PROPERTY(QSharedPointer<Player> re2Player READ re2Player WRITE setRe2Player)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:game",
                "reverserelation=rounds")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:drinks",
                "reverserelation=round")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:pointInstances",
                "reverserelation=round")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:schmeissereien",
                "reverserelation=round")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hochzeitPlayer",
                "reverserelation=hochzeitRounds")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:trumpfabgabePlayer",
                "reverserelation=trumpfabgabeRounds")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:soloPlayer",
                "reverserelation=soloRounds")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:schweinereiPlayer",
                "reverserelation=schweinereiRounds")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:re1Player",
                "reverserelation=re1Rounds")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:re2Player",
                "reverserelation=re2Rounds")
public:
    enum WinnerParty {
        Re = 1,
        Contra = 2,
        Draw = 3,
        UnknownWinnerParty = 4
    };

    enum SoloType {
        UnkownSoloType,
        NoSolo,
        Fleischlos,
        Buben,
        Damen,
        Trumpf,
        StilleHochzeit,
        FalschGespielt
    };

    explicit Round(QObject *parent = 0);
    ~Round();

    int number() const;
    void setNumber(int number);

    QDateTime startTime() const;

    QTime length() const;
    void setLength(const QTime &length);

    QString comment() const;
    void setComment(const QString &comment);

    int points() const;
    int points(QSharedPointer<Player> player) const;
    int totalPoints(QSharedPointer<Player> player) const;
    QList<QSharedPointer<Player> > playersSortedByPlacement() const;
    int placement(QSharedPointer<Player> player) const;

    QSharedPointer<Game> game() const;

    QList<QSharedPointer<LiveDrink> > drinks() const;

    QSharedPointer<Player> cardMixer() const;
    QList<QSharedPointer<Player> > playingPlayers() const;

//    DECLARE_MAPPINGATTRIBUTE_IN_CALC(Player*,bool,DokoRound,DokoRoundCalculator,doko_re)
//    DECLARE_LISTATTRIBUTE_IN_CALC(Player*,DokoRound,DokoRoundCalculator,doko_winningPlayers)
//    DECLARE_LISTATTRIBUTE_IN_CALC(QList<Player*>,DokoRound,DokoRoundCalculator,doko_togetherPlayingPlayers)

    QString soloTypeString() const;

    SoloType soloType() const;
    void setSoloType(const SoloType &soloType);

    bool isSolo() const;

    bool isPflicht() const;
    void setIsPflicht(bool isPflicht);

    WinnerParty winnerParty() const;
    void setWinnerParty(const WinnerParty &winnerParty);

    QSharedPointer<Player> hochzeitPlayer() const;
    void setHochzeitPlayer(const QSharedPointer<Player> &hochzeitPlayer);

    QSharedPointer<Player> trumpfabgabePlayer() const;
    void setTrumpfabgabePlayer(const QSharedPointer<Player> &trumpfabgabePlayer);

    QSharedPointer<Player> soloPlayer() const;
    void setSoloPlayer(const QSharedPointer<Player> &soloPlayer);

    QSharedPointer<Player> schweinereiPlayer() const;
    void setSchweinereiPlayer(const QSharedPointer<Player> &schweinereiPlayer);

    QSharedPointer<Player> re1Player() const;
    void setRe1Player(const QSharedPointer<Player> &re1Player);

    QSharedPointer<Player> re2Player() const;
    void setRe2Player(const QSharedPointer<Player> &re2Player);

    bool isRe(QSharedPointer<Player> player) const;

    QList<QSharedPointer<Player> > rePlayers() const;
    QList<QSharedPointer<Player> > contraPlayers() const;
    QList<QSharedPointer<Player> > winners() const;
    QList<QSharedPointer<Player> > losers() const;

    QList<QSharedPointer<Schmeisserei> > schmeissereien() const;

private:
    void setDrinks(const QList<QSharedPointer<LiveDrink> > &drinks);
    void setGame(const QSharedPointer<Game> &game);
    void setStartTime(const QDateTime &startTime);
    QList<QSharedPointer<Point> > pointInstances() const;
    void setPointInstances(const QList<QSharedPointer<Point> > &pointInstances);
    void setSchmeissereien(const QList<QSharedPointer<Schmeisserei> > &schmeissereien);

    QMap<int, int> _points() const;
    void setPoints(const QMap<int, int> &_points);

    int cardMixerPosition() const;
    void setSoloTypeString(const QString &soloTypeString);

    int m_number;
    QDateTime m_startTime;
    QTime m_length;
    QString m_comment;
    QString m_soloTypeString;
    SoloType m_soloType;
    bool m_soloIsPflicht;
    WinnerParty m_winnerParty;
    QMap<int, int> m_points;

    QpWeakRelation<Game> m_game;
    QpStrongRelation<LiveDrink> m_drinks;
    QpStrongRelation<Point> m_pointInstances;
    QpStrongRelation<Schmeisserei> m_schmeissereien;
    QpWeakRelation<Player> m_hochzeitPlayer;
    QpWeakRelation<Player> m_trumpfabgabePlayer;
    QpWeakRelation<Player> m_soloPlayer;
    QpWeakRelation<Player> m_schweinereiPlayer;
    QpWeakRelation<Player> m_re1Player;
    QpWeakRelation<Player> m_re2Player;
};

#undef COMMA

} // namespace NewDatabase

#endif // ROUND_H
