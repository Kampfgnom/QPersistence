#include "series.h"

Series::Series(QObject *parent) :
    QObject(parent),
    m_title(QLatin1String("")),
    m_absolutePath(QLatin1String("")),
    m_tvdbId(0),
    m_imdbId(QLatin1String("")),
    m_overview(QLatin1String("")),
    m_firstAired(QDate()),
    m_genres(QStringList()),
    m_actors(QStringList()),
    m_bannerUrls(QStringList()),
    m_posterUrls(QStringList())
{
}

Series::~Series()
{
}

QString Series::title() const
{
    return m_title;
}

void Series::setTitle(const QString &title)
{
    m_title = title;
}

QString Series::absolutePath() const
{
    return m_absolutePath;
}

void Series::setAbsolutePath(const QString &path)
{
    m_absolutePath = path;
}

int Series::tvdbId() const
{
    return m_tvdbId;
}

void Series::setTvdbId(int id)
{
    m_tvdbId = id;
}

QString Series::tvdbUrl() const
{
    return QString(QLatin1String("http://thetvdb.com/?tab=series&id=%1")).arg(tvdbId());
}

QString Series::imdbId() const
{
    return m_imdbId;
}

void Series::setImdbId(const QString &id)
{
    m_imdbId = id;
}

QString Series::imdbUrl() const
{
    return QString(QLatin1String("http://www.imdb.com/title/%1")).arg(imdbId());
}

QString Series::overview() const
{
    return m_overview;
}

void Series::setOverview(const QString &overview)
{
    m_overview = overview;
}

QDate Series::firstAired() const
{
    return m_firstAired;
}

void Series::setFirstAired(const QDate &date)
{
    m_firstAired = date;
}

QStringList Series::genres() const
{
    return m_genres;
}

void Series::setGenres(const QStringList &genres)
{
    m_genres = genres;
}

QStringList Series::actors() const
{
    return m_actors;
}

void Series::setActors(const QStringList &actors)
{
    m_actors = actors;
}

QStringList Series::bannerUrls() const
{
    return m_bannerUrls;
}

void Series::setBannerUrls(const QStringList &urls)
{
    m_bannerUrls = urls;
}

QStringList Series::posterUrls() const
{
    return m_posterUrls;
}

void Series::setPosterUrls(const QStringList &urls)
{
    m_posterUrls = urls;
}

QList<Season *> Series::seasons() const
{
    return m_seasons;
}


void Series::setSeasons(const QList<Season *> &seasons)
{
    m_seasons = seasons;
}
