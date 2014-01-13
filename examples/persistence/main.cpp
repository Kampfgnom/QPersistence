#include <QApplication>

#include <testModel/parentobject.h>
#include <testModel/childobject.h>

#include <QPersistence.h>

#include <QDebug>
#include <QSqlError>
#include <QPluginLoader>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("192.168.100.100");
    db.setDatabaseName("niklas");
    db.setUserName("niklas");
    db.setPassword("niklas");
    if(!db.open()) {
        qWarning() << db.lastError();
        return 0;
    }

    Qp::setSqlDebugEnabled(true);
    Qp::setDatabase(db);
    Qp::registerClass<ParentObject>();
    Qp::registerClass<ChildObject>();
    Qp::createCleanSchema();

    if(Qp::lastError().isValid()) {
        qWarning() << Qp::lastError();
        return 0;
    }
//    QSharedPointer<ChildObject> child1 = Qp::create<ChildObject>();
//    qDebug() << Qp::creationTime(child1).toString();

//    qDebug() << "######################################";
//    {
//        QSharedPointer<ParentObject> parent1 = Qp::create<ParentObject>();
//        parent1->setChildObject(child1);
//        qDebug() << parent1.data();
//        qDebug() << Qp::creationTime(parent1).toString();
//        Qp::update(parent1);
//    }
//    qDebug() << "######################################";
//    {
//        QSharedPointer<ParentObject> p = child1->parentObjectOneToOne();
//        qDebug() << p.data();
//        Qp::remove(p);
//    }

//    qDebug() << "######################################";
//    {
//        QSharedPointer<ParentObject> p = child1->parentObjectOneToOne();
//        qDebug() << p.data();
//    }

//    Qp::remove(child1);

    {
        for(int i = 0; i < 2; ++i) {
            QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
            for(int j = 0; j < 4; ++j) {
                QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
                parent->addChildObjectManyToMany(child);
                Qp::update(child);
            }
        }
    }


    db.close();

    return a.exec();
}
