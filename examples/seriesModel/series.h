#ifndef SERIES_H
#define SERIES_H

#include <QObject>

#include <QPersistence.h>

#include <QDate>
#include <QStringList>

class Season;

class Series : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int tvdbId READ tvdbId WRITE setTvdbId)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString absolutePath READ absolutePath WRITE setAbsolutePath)
    Q_PROPERTY(QString imdbId READ imdbId WRITE setImdbId)
    Q_PROPERTY(QString overview READ overview WRITE setOverview)
    Q_PROPERTY(QDate firstAired READ firstAired WRITE setFirstAired)
    Q_PROPERTY(QStringList genres READ genres WRITE setGenres)
    Q_PROPERTY(QStringList actors READ actors WRITE setActors)
    Q_PROPERTY(QStringList bannerUrls READ bannerUrls WRITE setBannerUrls)
    Q_PROPERTY(QStringList posterUrls READ posterUrls WRITE setPosterUrls)
    Q_PROPERTY(QList<Season*> seasons READ seasons WRITE setSeasons)

    Q_CLASSINFO(QPERSISTENCE_PRIMARYKEY, "tvdbId")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:seasons",
                "reverserelation=series;")

public:
    explicit Series(QObject *parent = 0);
    ~Series();
    
    QString title() const;
    void setTitle(const QString &title);

    QString absolutePath() const;
    void setAbsolutePath(const QString &path);

    int tvdbId() const;
    void setTvdbId(int id);
    QString tvdbUrl() const;

    QString imdbId() const;
    void setImdbId(const QString &id);
    QString imdbUrl() const;

    QString overview() const;
    void setOverview(const QString &overview);

    QDate firstAired() const;
    void setFirstAired(const QDate &date);

    QStringList genres() const;
    void setGenres(const QStringList &genres);

    QStringList actors() const;
    void setActors(const QStringList &actors);

    QStringList bannerUrls() const;
    void setBannerUrls(const QStringList &urls);

    QStringList posterUrls() const;
    void setPosterUrls(const QStringList &urls);

    QList<Season *> seasons() const;
    void setSeasons(const QList<Season *> &seasons);

private:
    QString m_title;
    QString m_absolutePath;
    int m_tvdbId;
    QString m_imdbId;
    QString m_overview;
    QDate m_firstAired;
    QStringList m_genres;
    QStringList m_actors;
    QStringList m_bannerUrls;
    QStringList m_posterUrls;
    QList<Season *> m_seasons;
};

Q_DECLARE_METATYPE(Series*)

#endif // SERIES_H
