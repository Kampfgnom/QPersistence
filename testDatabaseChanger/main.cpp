#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/condition.h"
#include "../tests/tst_synchronizetest.h"
#include "../tests/tst_locktest.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtTest>
#include <QGuiApplication>

#include <QPersistence/legacysqldatasource.h>

#ifndef QP_NO_LOCKS
void lockedCounter(QSharedPointer<TestNameSpace::ParentObject> parent);
void lockedCounter(QSharedPointer<TestNameSpace::ParentObject> parent) {

    for(int i = 0; i < 100; ++i) {
        QTRY_COMPARE(Qp::tryLock(parent).status(), QpLock::LockedLocally);
        Qp::synchronize(parent);
        parent->increaseCounter();
        Qp::update(parent);
        Qp::unlock(parent);
    }
}
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#endif

int main(int argc, char *argv[])
{
    qDebug() << "Starting changer";
    QGuiApplication a(argc, argv);
#if !defined QP_NO_LOCKS || !defined QP_NO_TIMESTAMPS
    if(a.arguments().size() != 3) {
        qWarning() << "Usage: qpersistencetestdatabasechanger <id>";
        return -1;
    }

    int id = a.arguments().at(1).toInt();

    QSqlDatabase db = Qp::database();

    if(!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("boot2docker");
        db.setDatabaseName("qpersistence_testing");
        db.setUserName("qpersistence_testing");
        db.setPassword("qpersistence_testing");
        if(!db.open()) {
            qWarning() << db.lastError();
            return -1;
        }

        QpLegacySqlDatasource *ds = new QpLegacySqlDatasource(Qp::defaultStorage());
        ds->setSqlDatabase(db);
        Qp::defaultStorage()->setDatasource(ds);

        foreach(QString field, LockTest::additionalLockInfo().keys()) {
            Qp::addAdditionalLockInformationField(field);
        }

        Qp::enableLocks();
        Qp::setDatabase(db);
        Qp::setSqlDebugEnabled(false);
        Qp::registerClass<TestNameSpace::ParentObject>();
        Qp::registerClass<TestNameSpace::ChildObject>();
    }

    QTest::qSleep(1010);

    QSharedPointer<TestNameSpace::ParentObject> parent;
    if(id > 0)
        parent = Qp::read<TestNameSpace::ParentObject>(id);

    SynchronizeTest::ChangerMode mode = static_cast<SynchronizeTest::ChangerMode>(a.arguments().at(2).toInt());

    if(mode == SynchronizeTest::CreateAndUpdate) {
        QList<QSharedPointer<TestNameSpace::ParentObject>> list;
        qDebug() << "creating objects";
        for(int i = 0; i < id; ++i) {
            list << Qp::create<TestNameSpace::ParentObject>();
        }
        QTest::qSleep(2000);
        qDebug() << "creating more objects";
        for(int i = 0; i < id; ++i) {
            list << Qp::create<TestNameSpace::ParentObject>();
        }

        QTest::qSleep(1010);
        qDebug() << "updating objects";
        foreach(QSharedPointer<TestNameSpace::ParentObject> o, list) {
            o->setAString("test");
            Qp::update(o);
        }
    }
    else if(mode == SynchronizeTest::LockedCounting) {
        lockedCounter(parent);
    }
    else if(mode == SynchronizeTest::LockAndUnlock) {
        QTest::qSleep(1010);
        QHash<QString, QVariant> i = LockTest::additionalLockInfo();
        Qp::tryLock(parent, i);

        QTest::qSleep(1010);
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
            QTest::qSleep(1010);
        }
    }
    else if(mode == SynchronizeTest::OneToOne) {
        for(int i = 0; i < SynchronizeTest::childInts().size(); ++i) {
            QSharedPointer<TestNameSpace::ChildObject> oneToOneChild = Qp::create<TestNameSpace::ChildObject>();
            oneToOneChild->setSomeInt(SynchronizeTest::childInts().at(i));
            Qp::update(oneToOneChild);

            parent->setChildObjectOneToOne(oneToOneChild);
            Qp::update(parent);

            QTest::qSleep(1010);
        }

        parent->setChildObjectOneToOne(QSharedPointer<TestNameSpace::ChildObject>());
        Qp::update(parent);
    }
    else if(mode == SynchronizeTest::OneToMany) {
        for(int i = 0; i < SynchronizeTest::childInts().size(); ++i) {
            for(int indexOneToMany = 0; indexOneToMany < SynchronizeTest::childInts().size(); ++indexOneToMany) {
                QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
                child->setSomeInt(SynchronizeTest::childInts().at(indexOneToMany));
                Qp::update(child);

                parent->addChildObjectsOneToMany(child);
            }
            Qp::update(parent);

            QTest::qSleep(1010);
        }
    }
    else if(mode == SynchronizeTest::ManyToMany) {
        for(int i = 0; i < SynchronizeTest::childInts().size(); ++i) {
            for(int indexOneToMany = 0; indexOneToMany < SynchronizeTest::childInts().size(); ++indexOneToMany) {
                QSharedPointer<TestNameSpace::ChildObject> child = Qp::create<TestNameSpace::ChildObject>();
                child->setSomeInt(SynchronizeTest::childInts().at(indexOneToMany));
                Qp::update(child);

                parent->addChildObjectsManyToMany(child);
            }
            Qp::update(parent);

            QTest::qSleep(1010);
        }
    }
#endif

    return 0;
}
