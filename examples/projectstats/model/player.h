#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>

#include <QColor>
#include <QPixmap>
#include <QPersistenceRelations.h>

namespace NewDatabase {

class Schmeisserei;
class Round;
class Place;
class LiveDrink;
class Game;
class Point;

class Player : public QObject
{
    Q_OBJECT
    Q_ENUMS(Gender)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(Gender gender READ gender WRITE setGender)
    Q_PROPERTY(int weight READ weight WRITE setWeight)
    Q_PROPERTY(int size READ size WRITE setSize)
    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(QPixmap avatar READ avatar WRITE setAvatar)
    Q_PROPERTY(QList<QSharedPointer<Place> > places READ places WRITE setPlaces)
    Q_PROPERTY(QList<QSharedPointer<LiveDrink> > liveDrinks READ liveDrinks WRITE setLiveDrinks)
    Q_PROPERTY(QList<QSharedPointer<Game> > games READ games WRITE setGames)
//    Q_PROPERTY(QList<QSharedPointer<Point> > pointsInstances READ pointsInstances WRITE setPointInstances)
    Q_PROPERTY(QList<QSharedPointer<Schmeisserei> > schmeissereien READ schmeissereien WRITE setSchmeissereien)

    Q_PROPERTY(QList<QSharedPointer<Round> > hochzeitRounds READ hochzeitRounds WRITE setHochzeitRounds)
    Q_PROPERTY(QList<QSharedPointer<Round> > trumpfabgabeRounds READ trumpfabgabeRounds WRITE setTrumpfabgabeRounds)
    Q_PROPERTY(QList<QSharedPointer<Round> > soloRounds READ soloRounds WRITE setSoloRounds)
    Q_PROPERTY(QList<QSharedPointer<Round> > schweinereiRounds READ schweinereiRounds WRITE setSchweinereiRounds)
    Q_PROPERTY(QList<QSharedPointer<Round> > re1Rounds READ re1Rounds WRITE setRe1Rounds)
    Q_PROPERTY(QList<QSharedPointer<Round> > re2Rounds READ re2Rounds WRITE setRe2Rounds)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:places",
                "reverserelation=players")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:liveDrinks",
                "reverserelation=player")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:games",
                "reverserelation=players")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:points",
                "reverserelation=player")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:schmeissereien",
                "reverserelation=player")

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hochzeitRounds",
                "reverserelation=hochzeitPlayer")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:trumpfabgabeRounds",
                "reverserelation=trumpfabgabePlayer")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:soloRounds",
                "reverserelation=soloPlayer")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:schweinereiRounds",
                "reverserelation=schweinereiPlayer")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:re1Rounds",
                "reverserelation=re1Player")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:re2Rounds",
                "reverserelation=re2Player")

public:
    enum Gender {
        UnkownGender,
        Male,
        Female
    };

    explicit Player(QObject *parent = 0);
    ~Player();

    QString name() const;
    void setName(const QString &name);

    Gender gender() const;
    void setGender(const Gender &gender);

    int weight() const;
    void setWeight(int weight);

    int size() const;
    void setSize(int size);

    QColor color() const;
    void setColor(const QColor &color);

    QPixmap avatar() const;
    void setAvatar(const QPixmap &avatar);

    QList<QSharedPointer<Place> > places() const;
    void addPlace(QSharedPointer<Place> place);

    QList<QSharedPointer<LiveDrink> > liveDrinks() const;
    QList<QSharedPointer<Game> > games() const;

    int gamePoints() const;
    int points() const;
    double average() const;

    int wins() const;
    int losses() const;
    double averagePlacement() const;

    QSharedPointer<Game> lastGame() const;
    QSharedPointer<Game> lastWin() const;

//    DECLARE_MAPPINGATTRIBUTE_IN_CALC(LiveGame*,double,Player,PlayerCalculator,alcPegel)

    QList<QSharedPointer<Schmeisserei> > schmeissereien() const;
    QList<QSharedPointer<Round> > rounds() const;

    QList<QSharedPointer<Round> > hochzeitRounds() const;
    QList<QSharedPointer<Round> > trumpfabgabeRounds() const;
    QList<QSharedPointer<Round> > soloRounds() const;
    QList<QSharedPointer<Round> > schweinereiRounds() const;
    QList<QSharedPointer<Round> > re1Rounds() const;
    QList<QSharedPointer<Round> > re2Rounds() const;

    QList<QSharedPointer<Round> > reRounds() const;
    double rePercentage() const;
    QList<QSharedPointer<Round> > reWins() const;
    double reWinsPercentage() const;

    QList<QSharedPointer<Round> > contraRounds() const;
    double contraPercentage() const;
    QList<QSharedPointer<Round> > contraWins() const;
    double contraWinsPercentage() const;

    QList<QSharedPointer<Round> > hochzeitenRounds() const;
    QList<QSharedPointer<Round> > soliRounds() const;
    QList<QSharedPointer<Round> > trumpfabgabenRounds() const;
    QList<QSharedPointer<Round> > trumpfabgabenAbgegebenRounds() const;
    QList<QSharedPointer<Round> > trumpfabgabenAufgenommenRounds() const;
    QList<QSharedPointer<Round> > schweinereienRounds() const;
    QList<QSharedPointer<Round> > schmeissereienRounds() const;

    double averagePointsPerRound() const;
    QList<QSharedPointer<Round> > winRounds() const;
    double roundWinsPercentage() const;

private:
    void setPlaces(const QList<QSharedPointer<Place> > &places);
    void setLiveDrinks(const QList<QSharedPointer<LiveDrink> > &drinks);
    void setGames(const QList<QSharedPointer<Game> > &games);
    void setSchmeissereien(const QList<QSharedPointer<Schmeisserei> > &schmeissereien);
    void setHochzeitRounds(const QList<QSharedPointer<Round> > &hochzeitRounds);
    void setTrumpfabgabeRounds(const QList<QSharedPointer<Round> > &trumpfabgabeRounds);
    void setSoloRounds(const QList<QSharedPointer<Round> > &soloRounds);
    void setSchweinereiRounds(const QList<QSharedPointer<Round> > &schweinereiRounds);
    void setRe1Rounds(const QList<QSharedPointer<Round> > &re1Rounds);
    void setRe2Rounds(const QList<QSharedPointer<Round> > &re2Rounds);

    QList<QSharedPointer<Point> > pointsInstances() const;
    void setPointInstances(const QList<QSharedPointer<Point> > &pointInstances);

    QString m_name;
    Gender m_gender;
    int m_weight;
    int m_size;
    QColor m_color;
    QPixmap m_avatar;

    QpWeakRelation<Place> m_places;
    QpWeakRelation<LiveDrink> m_liveDrinks;
    QpWeakRelation<Game> m_games;
    QpStrongRelation<Point> m_points;
    QpStrongRelation<Schmeisserei> m_schmeissereien;

    QpWeakRelation<Round> m_hochzeitRounds;
    QpWeakRelation<Round> m_trumpfabgabeRounds;
    QpWeakRelation<Round> m_soloRounds;
    QpWeakRelation<Round> m_schweinereiRounds;
    QpWeakRelation<Round> m_re1Rounds;
    QpWeakRelation<Round> m_re2Rounds;
};

} // namespace NewDatabase

#endif // PLAYER_H
