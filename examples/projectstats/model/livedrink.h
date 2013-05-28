#ifndef LIVEDRINK_H
#define LIVEDRINK_H

#include <QObject>

#include <QPersistenceRelations.h>
#include <QDateTime>

namespace NewDatabase {

class Player;
class Round;
class Drink;

class LiveDrink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDateTime time READ time WRITE setTime)
    Q_PROPERTY(QSharedPointer<Player> player READ player WRITE setPlayer)
    Q_PROPERTY(QSharedPointer<Round> round READ round WRITE setRound)
    Q_PROPERTY(QSharedPointer<Drink> drink READ drink WRITE setDrink)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:player",
                "reverserelation=liveDrinks")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:round",
                "reverserelation=drinks")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:drink",
                "reverserelation=liveDrinks")

public:
    explicit LiveDrink(QObject *parent = 0);
    ~LiveDrink();

    QDateTime time() const;

    QSharedPointer<Player> player() const;
    QSharedPointer<Round> round() const;
    QSharedPointer<Drink> drink() const;

private:
    void setPlayer(QSharedPointer<Player> player);
    void setRound(QSharedPointer<Round> round);
    void setDrink(QSharedPointer<Drink> drink);
    void setTime(const QDateTime &time);

    QDateTime m_time;

    QpWeakRelation<Player> m_player;
    QpWeakRelation<Round> m_round;
    QpWeakRelation<Drink> m_drink;
};

} // namespace NewDatabase

#endif // LIVEDRINK_H
