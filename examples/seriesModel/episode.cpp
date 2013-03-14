#include "episode.h"

Episode::Episode(QObject *parent) :
    QObject(parent),
    m_length(QTime(0,0)),
    m_title(QLatin1String("")),
    m_absoluteFileName(QLatin1String("")),
    m_number(0),
    m_tvdbId(0),
    m_imdbId(QLatin1String("")),
    m_overview(QLatin1String("")),
    m_firstAired(QDate()),
    m_guestStars(QStringList()),
    m_writers(QStringList()),
    m_director(QLatin1String(""))
{
}

QTime Episode::length() const
{
    return m_length;
}

void Episode::setLength(const QTime &length)
{
    m_length = length;
}

QString Episode::title() const
{
    return m_title;
}

void Episode::setTitle(const QString &title)
{
    m_title = title;
}

QString Episode::absoluteFileName() const
{
    return m_absoluteFileName;
}

void Episode::setAbsoluteFileName(const QString filename)
{
    m_absoluteFileName = filename;
}

int Episode::number() const
{
    return m_number;
}

void Episode::setNumber(int number)
{
    m_number = number;
}

int Episode::tvdbId() const
{
    return m_tvdbId;
}

void Episode::setTvdbId(int id)
{
    m_tvdbId = id;
}

QString Episode::imdbId() const
{
    return m_imdbId;
}

void Episode::setImdbId(const QString &imdbId)
{
    m_imdbId = imdbId;
}

QString Episode::overview() const
{
    return m_overview;
}

void Episode::setOverview(const QString &overview)
{
    m_overview = overview;
}

QDate Episode::firstAired() const
{
    return m_firstAired;
}

void Episode::setFirstAired(const QDate &date)
{
    m_firstAired = date;
}

QStringList Episode::guestStars() const
{
    return m_guestStars;
}

void Episode::setGuestStars(const QStringList &guestStars)
{
    m_guestStars = guestStars;
}
QStringList Episode::writers() const
{
    return m_writers;
}

void Episode::setWriters(const QStringList &writers)
{
    m_writers = writers;
}

QString Episode::director() const
{
    return m_director;
}

void Episode::setDirector(const QString &director)
{
    m_director = director;
}
