
#include <QApplication>

#include <seriesModel/seriesmodel.h>

#include <QDataSuite/simpledataaccessobject.h>
#include <QDataSuite/error.h>
#include <QRestServer/server.h>

#include <QDebug>
#include <QSqlError>
#include <QUrl>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Register types
    QDataSuite::registerMetaObject<Series>();
    QDataSuite::registerMetaObject<Season>();
    QDataSuite::SimpleDataAccessObject<Series> seriesDao;
    QDataSuite::SimpleDataAccessObject<Season> seasonDao;

    // Series
    QList<int> seriesids;
    seriesids << 1  // Simpsons
              << 2; // white collar

    // Seasons
    QList<QList<int> > seasons;
    seasons << (QList<int>() << 11 << 12); // Simpsons
    seasons << (QList<int>() << 21 << 22); // White collar

    // Insert
    for(int i = 0; i < seriesids.size(); ++i) {
        Series *series = seriesDao.create();
        series->setTvdbId(seriesids.at(i));

        if(!seriesDao.insert(series)) {
            qCritical() << seriesDao.lastError();
        }

        QList<Season *> seasonList;
        QList<int> seasonIds = seasons.at(i);
        for(int j = 0; j < seasonIds.size(); ++j) {
            Season *season = seasonDao.create();
            season->setTvdbId(seasonIds.at(j));
            season->setSeries(series);
            seasonList << season;

            if(!seasonDao.insert(season)) {
                qCritical() << seasonDao.lastError();
            }
        }

        series->setSeasons(seasonList);
        if(!seriesDao.update(series)) {
            qCritical() << seriesDao.lastError();
        }
    }

    QRestServer::Server server;
    server.setBaseUrl(QUrl("http://localhost"));
    server.addCollection(&seriesDao);
    server.addCollection(&seasonDao);
    server.listen(8080);

    return a.exec();
}
