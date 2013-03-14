#ifndef SEASON_H
#define SEASON_H

#include <QObject>

#include <QPersistence.h>

class Series;

class Season : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int tvdbId READ tvdbId WRITE setTvdbId)
    Q_PROPERTY(int number READ number WRITE setNumber)
    Q_PROPERTY(QString absolutePath READ absolutePath WRITE setAbsolutePath)
    Q_PROPERTY(Series* series READ series WRITE setSeries)

    Q_CLASSINFO(QPERSISTENCE_PRIMARYKEY, "tvdbId")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:series",
                "reverserelation=seasons;")

public:
    explicit Season(QObject *parent = 0);

    int tvdbId() const;
    void setTvdbId(int id);

    int number() const;
    void setNumber(int number);

    QString absolutePath() const;
    void setAbsolutePath(const QString &path);

    Series *series() const;
    void setSeries(Series *series);

private:
    int m_number;
    int m_tvdbId;
    QString m_absolutePath;
    Series *m_series;
};

Q_DECLARE_METATYPE(Season*)

#endif // SEASON_H
