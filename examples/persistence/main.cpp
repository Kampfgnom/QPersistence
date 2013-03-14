#include <QApplication>

#include <seriesModel/seriesmodel.h>

#include <QPersistenceDatabaseSchema.h>
#include <QPersistencePersistentDataAccessObject.h>
#include <QPersistenceError.h>

#include <QDebug>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("test.sqlite");
    if(!db.open()) {
        qCritical() << db.lastError();
    }

    // Register types
    QPersistencePersistentDataAccessObject<Series> seriesDao;
    QPersistencePersistentDataAccessObject<Season> seasonDao;
    QPersistence::registerDataAccessObject<Series>(&seriesDao);
    QPersistence::registerDataAccessObject<Season>(&seasonDao);

    // Drop and create tables
    QPersistenceDatabaseSchema databaseSchema(db);
//    databaseSchema.adjustSchema();
    databaseSchema.createCleanSchema();

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
        QScopedPointer<Series> s(series);
        series->setTvdbId(seriesids.at(i));

        if(!seriesDao.insert(series)) {
            qCritical() << seriesDao.lastError();
        }

        QList<int> seasonIds = seasons.at(i);
        for(int j = 0; j < seasonIds.size(); ++j) {
            Season *season = seasonDao.create();
            QScopedPointer<Season> se(season);
            season->setTvdbId(seasonIds.at(j));
            season->setSeries(series);

            if(!seasonDao.insert(season)) {
                qCritical() << seasonDao.lastError();
            }
        }
    }

    // Update
    foreach(Season *season, seasonDao.readAll()) {
        QScopedPointer<Season> s(season);
        season->setNumber(123);
        qDebug() << season->series();
        seasonDao.update(season);
    }

    foreach(Series *series, seriesDao.readAll()) {
        QScopedPointer<Series> s(series);
        series->setImdbId("asd");
        seriesDao.update(series);

        foreach(Season *season, series->seasons()) {
            season->setNumber(123);
            seasonDao.update(season);
        }
    }

    // Delete
    foreach(Series *series, seriesDao.readAll()) {
        QScopedPointer<Series> s(series);
        seriesDao.remove(series);
    }

    foreach(Season *season, seasonDao.readAll()) {
        QScopedPointer<Season> s(season);
        seasonDao.remove(season);
    }
    return a.exec();
}
