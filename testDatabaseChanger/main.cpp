#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"
#include <QtTest>
#include "../tests/tst_synchronizetest.h"

#include <QGuiApplication>

void lockedCounter(QSharedPointer<ParentObject> parent) {

    for(int i = 0; i < 100; ++i) {
        QTRY_COMPARE(Qp::tryLock(parent).status(), QpLock::LockedLocally);
        Qp::synchronize(parent);
        parent->increaseCounter();
        Qp::update(parent);
        Qp::unlock(parent);
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);
    if(a.arguments().size() != 3) {
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

    QTest::qSleep(1000);

    QSharedPointer<ParentObject> parent = Qp::read<ParentObject>(id);

    SynchronizeTest::ChangerMode mode = static_cast<SynchronizeTest::ChangerMode>(a.arguments().at(2).toInt());

    if(mode == SynchronizeTest::LockedCounting) {
        lockedCounter(parent);
    }
    else if(mode == SynchronizeTest::LockAndUnlock) {
        QTest::qSleep(1000);
        Qp::tryLock(parent);
        QTest::qSleep(1000);
        Qp::unlock(parent);
    }
    else if(mode == SynchronizeTest::ChangeOnce) {
        parent->increaseCounter();
        Qp::update(parent);
    }
    else if(mode == SynchronizeTest::Counter) {
        for(int i = 0; i < SynchronizeTest::childInts().size(); ++i) {
            parent->increaseCounter();
            Qp::update(parent);
            QTest::qSleep(1000);
        }
    }
    else if(mode == SynchronizeTest::OneToOne) {
        for(int i = 0; i < SynchronizeTest::childInts().size(); ++i) {
            QSharedPointer<ChildObject> oneToOneChild = Qp::create<ChildObject>();
            oneToOneChild->setSomeInt(SynchronizeTest::childInts().at(i));
            Qp::update(oneToOneChild);

            parent->setChildObjectOneToOne(oneToOneChild);
            Qp::update(parent);

            QTest::qSleep(1000);
        }

        parent->setChildObjectOneToOne(QSharedPointer<ChildObject>());
        Qp::update(parent);
    }
    else if(mode == SynchronizeTest::OneToMany) {
        for(int i = 0; i < SynchronizeTest::childInts().size(); ++i) {
            for(int indexOneToMany = 0; indexOneToMany < SynchronizeTest::childInts().size(); ++indexOneToMany) {
                QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
                child->setSomeInt(SynchronizeTest::childInts().at(indexOneToMany));
                Qp::update(child);

                parent->addChildObjectOneToMany(child);
            }
            Qp::update(parent);

            QTest::qSleep(1000);
        }
    }
    else if(mode == SynchronizeTest::ManyToMany) {
        for(int i = 0; i < SynchronizeTest::childInts().size(); ++i) {
            for(int indexOneToMany = 0; indexOneToMany < SynchronizeTest::childInts().size(); ++indexOneToMany) {
                QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
                child->setSomeInt(SynchronizeTest::childInts().at(indexOneToMany));
                Qp::update(child);

                parent->addChildObjectManyToMany(child);
            }
            Qp::update(parent);

            QTest::qSleep(1000);
        }
    }

    return 0;
}
