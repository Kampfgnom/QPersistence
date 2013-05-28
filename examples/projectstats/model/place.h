#ifndef PLACE_H
#define PLACE_H

#include <QObject>

#include <QPixmap>
#include <QPersistenceRelations.h>

namespace NewDatabase {

class Player;
class Game;

class Place : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int postalCode READ postalCode WRITE setPostalCode)
    Q_PROPERTY(QString city READ city WRITE setCity)
    Q_PROPERTY(QString street READ street WRITE setStreet)
    Q_PROPERTY(int houseNumber READ houseNumber WRITE setHouseNumber)
    Q_PROPERTY(QString comment READ comment WRITE setComment)
    Q_PROPERTY(QPixmap cityEmblem READ cityEmblem WRITE setCityEmblem)
    Q_PROPERTY(QList<QSharedPointer<Player> > players READ players WRITE setPlayers)
    Q_PROPERTY(QList<QSharedPointer<Game> > games READ games WRITE setGames)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:players",
                "reverserelation=places")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:games",
                "reverserelation=site")

public:
    explicit Place(QObject *parent = 0);
    ~Place();

    int postalCode() const;
    void setPostalCode(int postalCode);

    QString city() const;
    void setCity(const QString &city);

    QPixmap cityEmblem() const;
    void setCityEmblem(const QPixmap &cityEmblem);

    QString street() const;
    void setStreet(const QString &street);

    int houseNumber() const;
    void setHouseNumber(int houseNumber);

    QString comment() const;
    void setComment(const QString &comment);

    QList<QSharedPointer<Player> > players() const;

    QList<QSharedPointer<Game> > games() const;

    QString displayString() const;

private:
    friend class Player;
    void addPlayer(QSharedPointer<Player> player);

    void setPlayers(const QList<QSharedPointer<Player> > &players);
    void setGames(const QList<QSharedPointer<Game> > &games);

    int m_postalCode;
    QString m_city;
    QPixmap m_cityEmblem;
    QString m_street;
    int m_houseNumber;
    QString m_comment;

    QpWeakRelation<Player> m_players;
    QpWeakRelation<Game> m_games;
};

} // namespace NewDatabase

#endif // PLACE_H
