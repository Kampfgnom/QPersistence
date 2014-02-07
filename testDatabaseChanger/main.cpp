#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"
#include <QtTest>

#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    if(a.arguments().size() != 2) {
        qWarning() << "Usage: qpersistencetestdatabasechanger <id>";
        return -1;
    }

    int id = a.arguments().at(1).toInt();

    QSqlDatabase db = Qp::database();

    if(!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("192.168.100.2");
        db.setDatabaseName("niklas");
        db.setUserName("niklas");
        db.setPassword("niklas");

        Qp::setDatabase(db);
        Qp::setSqlDebugEnabled(false);
        Qp::registerClass<ParentObject>();
        Qp::registerClass<ChildObject>();
    }

    forever {
        QSharedPointer<ParentObject> parent = Qp::read<ParentObject>(id);
        parent->increaseCounter();
        Qp::update(parent);

        qDebug() << "Counter:" << parent->counter();

        QTest::qSleep(1000);
    }

    return 0;
}
