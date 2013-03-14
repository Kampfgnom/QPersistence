#include <QApplication>

#include <seriesModel/seriesmodel.h>

#include <QDataSuite/simpledataaccessobject.h>
#include <QDataSuite/error.h>

#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Series
    QDataSuite::SimpleDataAccessObject<Series> seriesSimpleDAO;

    QStringList seriesTitles;
    seriesTitles << "The Simpsons" << "White Collar" << "Game of Thrones";
    foreach(QString seriesTitle, seriesTitles) {
        Series *series = seriesSimpleDAO.create();
        series->setTitle(seriesTitle);

        if(!seriesSimpleDAO.insert(series)) {
            qCritical() << seriesSimpleDAO.lastError();
        }
    }

    foreach(Series *series, seriesSimpleDAO.readAll()) {
        if(seriesSimpleDAO.lastError().isValid()) {
            qCritical() << seriesSimpleDAO.lastError();
        }

        qDebug() << series->title();
    }


    return a.exec();
}
