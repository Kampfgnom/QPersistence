#include "mainwindow.h"
#include <QApplication>

#include "model/drink.h"
#include "model/game.h"
#include "model/livedrink.h"
#include "model/place.h"
#include "model/player.h"
#include "model/round.h"
#include "model/point.h"
#include "model/schmeisserei.h"

#include <QSqlDatabase>
#include <QStyle>
#include <QPersistence.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/Users/niklaswulf/Downloads/projectstats (1).db");
    db.open();

    Qp::registerMappableTypes<int, int>();
    Qp::setDatabase(db);
    Qp::registerClass<NewDatabase::Drink>();
    Qp::registerClass<NewDatabase::Game>();
    Qp::registerClass<NewDatabase::LiveDrink>();
    Qp::registerClass<NewDatabase::Place>();
    Qp::registerClass<NewDatabase::Player>();
    Qp::registerClass<NewDatabase::Round>();
//    Qp::registerClass<Point>();
    Qp::registerClass<NewDatabase::Schmeisserei>();
//        Qp::createCleanSchema();

    MainWindow w;
    w.show();

    return a.exec();
}
