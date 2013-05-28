#include <QApplication>

#include <testModel/parentobject.h>
#include <testModel/childobject.h>

#include <QPersistence.h>

#include <QDebug>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("test.sqlite");
    db.open();

    Qp::setDatabase(db);
    Qp::registerClass<ParentObject>();
    Qp::registerClass<ChildObject>();
    Qp::createCleanSchema();

    QSharedPointer<ChildObject> child1 = Qp::create<ChildObject>();

    qDebug() << "######################################";
    {
        QSharedPointer<ParentObject> parent1 = Qp::create<ParentObject>();
        parent1->setChildObject(child1);
        qDebug() << parent1.data();
        Qp::update(parent1);
    }

    qDebug() << "######################################";
    {
        QSharedPointer<ParentObject> p = child1->parentObject();
        qDebug() << p.data();
        Qp::remove(p);
    }


    qDebug() << "######################################";
    {
        QSharedPointer<ParentObject> p = child1->parentObject();
        qDebug() << p.data();
    }


    Qp::remove(child1);

    db.close();

    return a.exec();
}
