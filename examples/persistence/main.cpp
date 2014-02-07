#include <QApplication>

#include <parentobject.h>
#include <childobject.h>

#include <QPersistence.h>

#include <QDebug>
#include <QSqlError>
#include <QPluginLoader>
#include <QTest>

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

    qDebug() << "Inserting objects...";
    for(int i = 0; i < 1; ++i) {
        QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
        qDebug() << "## addChildObjectManyToMany #############################";
        for(int j = 0; j < 2; ++j) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            parent->addChildObjectManyToMany(child);
            Qp::update(child);
        }
        qDebug() << "## setChildObject #############################";
        for(int j = 0; j < 2; ++j) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            parent->setChildObjectOneToOne(child);
            Qp::update(child);
        }
        for(int j = 0; j < 2; ++j) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            parent->addChildObjectOneToMany(child);
            Qp::update(child);
        }
    }
    qDebug() << "Done.\n";

    QTest::qWait(3000);

    qDebug() << "Updating parents...";
    foreach(QSharedPointer<ParentObject> parent, Qp::readAll<ParentObject>()) {
        qDebug() << Qp::updateTimeInDatabase(parent);
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsManyToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsOneToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        qDebug() << Qp::updateTimeInDatabase(parent->childObjectOneToOne());

        parent->setAString(QString("test"));
        Qp::update(parent);

        qDebug() << Qp::updateTimeInDatabase(parent);
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsManyToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsOneToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        qDebug() << Qp::updateTimeInDatabase(parent->childObjectOneToOne());
    }
    qDebug() << "Done.\n";

    QTest::qWait(3000);

    qDebug() << "Updating m:n child...";
    foreach(QSharedPointer<ParentObject> parent, Qp::readAll<ParentObject>()) {
        qDebug() << Qp::updateTimeInDatabase(parent);
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsManyToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsOneToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        qDebug() << Qp::updateTimeInDatabase(parent->childObjectOneToOne());

        QSharedPointer<ChildObject> c1 = parent->childObjectsManyToMany().first();
        c1->setSomeInt(123);
        Qp::update(c1);

        qDebug() << Qp::updateTimeInDatabase(parent);
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsManyToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        foreach(QSharedPointer<ChildObject> c, parent->childObjectsOneToMany()) {
            qDebug() << Qp::updateTimeInDatabase(c);
        }
        qDebug() << Qp::updateTimeInDatabase(parent->childObjectOneToOne());
    }
    qDebug() << "Done.\n";

    db.close();

    return a.exec();
}
