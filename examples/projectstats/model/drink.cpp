#include "drink.h"

#include "livedrink.h"

namespace NewDatabase {

Drink::Drink(QObject *parent) :
    QObject(parent),
    m_liveDrinks("liveDrinks", this)
{
}

Drink::~Drink()
{
}

double Drink::alcoholByVolume() const
{
    return m_alcoholByVolume;
}

void Drink::setAlcoholByVolume(double alcoholByVolume)
{
    m_alcoholByVolume = alcoholByVolume;
}

QList<QSharedPointer<LiveDrink> > Drink::liveDrinks() const
{
    return m_liveDrinks.resolveList();
}

void Drink::setLiveDrinks(const QList<QSharedPointer<LiveDrink> > &drinks)
{
    m_liveDrinks.clear();
    m_liveDrinks.relate(drinks);
}

int Drink::count() const
{
    return liveDrinks().size();
}

int Drink::count(QSharedPointer<Player> player) const
{
    int count = 0;
    foreach(QSharedPointer<LiveDrink> drink, liveDrinks()) {
        if(drink->player() == player) {
            ++count;
        }
    }
    return count;
}

QList<QSharedPointer<Player> > Drink::playersSortedByCount() const
{
    QMap<QSharedPointer<Player>, int> counts;

    foreach(QSharedPointer<LiveDrink> drink, liveDrinks()) {
        ++counts[drink->player()];
    }

    QMultiMap<int, QSharedPointer<Player> > helperMap;
    for(auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        helperMap.insert(it.value(), it.key());
    }

    return helperMap.values();
}


QPixmap Drink::picture() const
{
    return m_picture;
}

void Drink::setPicture(const QPixmap &picture)
{
    m_picture = picture;
}

double Drink::volume() const
{
    return m_volume;
}

void Drink::setVolume(double volume)
{
    m_volume = volume;
}

Drink::Type Drink::type() const
{
    return m_type;
}

void Drink::setType(const Type &type)
{
    m_type = type;
}

QString Drink::name() const
{
    return m_name;
}

void Drink::setName(const QString &name)
{
    m_name = name;
}

} // namespace NewDatabase
