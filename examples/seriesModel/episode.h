#ifndef EPISODE_H
#define EPISODE_H

#include <QObject>

#include <QDate>
#include <QStringList>
#include <QTime>

class Episode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QTime length READ length WRITE setLength)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString absoluteFileName READ absoluteFileName WRITE setAbsoluteFileName)
    Q_PROPERTY(int number READ number WRITE setNumber)
    Q_PROPERTY(int tvdbId READ tvdbId WRITE setTvdbId)
    Q_PROPERTY(QString imdbId READ imdbId WRITE setImdbId)
    Q_PROPERTY(QString overview READ overview WRITE setOverview)
    Q_PROPERTY(QDate firstAired READ firstAired WRITE setFirstAired)
    Q_PROPERTY(QStringList guestStars READ guestStars WRITE setGuestStars)
    Q_PROPERTY(QStringList writers READ writers WRITE setWriters)
    Q_PROPERTY(QString director READ director WRITE setDirector)

public:
    explicit Episode(QObject *parent = 0);

    QTime length() const;
    void setLength(const QTime &length);

    QString title() const;
    void setTitle(const QString &title);

    QString absoluteFileName() const;
    void setAbsoluteFileName(const QString filename);

    int number() const;
    void setNumber(int number);

    int tvdbId() const;
    void setTvdbId(int id);

    QString imdbId() const;
    void setImdbId(const QString &imdbId);

    QString overview() const;
    void setOverview(const QString &overview);

    QDate firstAired() const;
    void setFirstAired(const QDate &date);

    QStringList guestStars() const;
    void setGuestStars(const QStringList &guestStars);

    QStringList writers() const;
    void setWriters(const QStringList &writers);

    QString director() const;
    void setDirector(const QString &director);

private:
    QTime m_length;
    QString m_title;
    QString m_absoluteFileName;
    int m_number;
    int m_tvdbId;
    QString m_imdbId;
    QString m_overview;
    QDate m_firstAired;
    QStringList m_guestStars;
    QStringList m_writers;
    QString m_director;
};

Q_DECLARE_METATYPE(Episode*)

#endif // EPISODE_H
