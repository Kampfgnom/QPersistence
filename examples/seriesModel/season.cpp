#include "season.h"

Season::Season(QObject *parent) :
    QObject(parent),
    m_number(0),
    m_absolutePath(QLatin1String("")),
    m_series(nullptr)
{
}

int Season::tvdbId() const
{
    return m_tvdbId;
}

void Season::setTvdbId(int id)
{
    m_tvdbId = id;
}

int Season::number() const
{
    return m_number;
}

void Season::setNumber(int number)
{
    m_number = number;
}

QString Season::absolutePath() const
{
    return m_absolutePath;
}

void Season::setAbsolutePath(const QString &path)
{
    m_absolutePath = path;
}

Series *Season::series() const
{
    return m_series;
}

void Season::setSeries(Series *series)
{
    m_series = series;
}
