#include "tst_synchronizetest.h"

#include <QProcess>
#include <QSqlError>

SynchronizeTest::SynchronizeTest(QObject *parent) :
    QObject(parent)
{
}

void SynchronizeTest::initDatabase()
{
    QSqlDatabase db = Qp::database();
    if(!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("192.168.100.2");
        db.setDatabaseName("niklas");
        db.setUserName("niklas");
        db.setPassword("niklas");

        QVERIFY2(db.open(), db.lastError().text().toUtf8());

        Qp::setDatabase(db);
        Qp::setSqlDebugEnabled(false);
        Qp::registerClass<ParentObject>();
        Qp::registerClass<ChildObject>();
        Qp::createCleanSchema();
    }
}

void SynchronizeTest::testUpdateTimeChangesFromOtherProcess()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

    QProcess *process = changerProcess(Qp::primaryKey(parent));
    process->start();

    QDateTime updateTime = Qp::updateTimeInDatabase(parent);

    const int COUNTS = 3;
    for(int i = 1; i <= COUNTS; ++i) {
        qDebug() << "Checking update time...";
        QTRY_COMPARE(updateTime.addSecs(i), Qp::updateTimeInDatabase(parent));
    }

    process->close();
    delete process;
}

void SynchronizeTest::testUnchangedSynchronizeResult()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    Qp::SynchronizeResult result = Qp::synchronize(parent);
    QCOMPARE(result, Qp::Unchanged);

    Qp::update(parent);

    result = Qp::synchronize(parent);
    QCOMPARE(result, Qp::Unchanged);
}

void SynchronizeTest::testSynchronizeCounter()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

    QProcess *process = changerProcess(Qp::primaryKey(parent));
    process->start();

    QDateTime updateTime = Qp::updateTimeInDatabase(parent);

    const int COUNTS = 3;
    for(int i = 1; i <= COUNTS; ++i) {
        QTRY_COMPARE(updateTime.addSecs(i), Qp::updateTimeInDatabase(parent));

        Qp::SynchronizeResult result = Qp::synchronize(parent);
        QCOMPARE(result, Qp::Updated);

        qDebug() << "Testing counter...";

        QCOMPARE(i + 1, parent->counter());
    }

    process->close();
    delete process;
}

QProcess *SynchronizeTest::changerProcess(int id)
{
    QProcess *process = new QProcess(this);
    process->setProgram("../testDatabaseChanger/qpersistencetestdatabasechanger");
    process->setArguments(QStringList() << QString("%1").arg(id));

    return process;
}
