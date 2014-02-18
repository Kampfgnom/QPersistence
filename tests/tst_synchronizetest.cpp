#include "tst_synchronizetest.h"

#include <QProcess>
#include <QSqlError>

QProcess *SynchronizeTest::m_currentProcess(nullptr);

SynchronizeTest::SynchronizeTest(QObject *parent) :
    QObject(parent)
{
}

void SynchronizeTest::cleanup(QProcess *process)
{
    Q_ASSERT(process == m_currentProcess);

    m_currentProcess->close();
    delete m_currentProcess;
    m_currentProcess = nullptr;
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

    QDateTime updateTime = Qp::updateTimeInDatabase(parent);

    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(Qp::primaryKey(parent), Counter));

    for(int i = 0; i < childInts().size(); ++i) {
        qDebug() << "Checking update time...";
        QTRY_VERIFY(updateTime < Qp::updateTimeInDatabase(parent));
        updateTime = Qp::updateTimeInDatabase(parent);
    }
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
    QDateTime lastUpdateTime = Qp::updateTimeInDatabase(parent);

    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(Qp::primaryKey(parent), Counter));

    for(int i = 0; i < childInts().size(); ++i) {
        qDebug() << "Testing counter...";
        QTRY_VERIFY(lastUpdateTime < Qp::updateTimeInDatabase(parent));
        lastUpdateTime = Qp::updateTimeInDatabase(parent);

        Qp::SynchronizeResult result = Qp::synchronize(parent);
        QCOMPARE(result, Qp::Updated);

        QCOMPARE(parent->counter(), i + 1);
    }
}

void SynchronizeTest::testSynchronizeOneToOneRelation()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

    QDateTime lastUpdateTime = Qp::updateTimeInDatabase(parent);

    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(Qp::primaryKey(parent), OneToOne));

    for(int i = 0; i < childInts().size(); ++i) {
        qDebug() << "Testing child...";
        QTRY_VERIFY(lastUpdateTime < Qp::updateTimeInDatabase(parent));
        lastUpdateTime = Qp::updateTimeInDatabase(parent);

        Qp::SynchronizeResult result = Qp::synchronize(parent);
        QCOMPARE(result, Qp::Updated);

        QSharedPointer<ChildObject> child = parent->childObjectOneToOne();
        QTRY_VERIFY(lastUpdateTime == Qp::updateTimeInDatabase(child));

        Qp::synchronize(child);
        QCOMPARE(child->someInt(), childInts().at(i));
    }

    QTRY_VERIFY(lastUpdateTime < Qp::updateTimeInDatabase(parent));
    Qp::SynchronizeResult result = Qp::synchronize(parent);
    QCOMPARE(result, Qp::Updated);

    QCOMPARE(QSharedPointer<ChildObject>(), parent->childObjectOneToOne());
}

void SynchronizeTest::testSynchronizeOneToManyRelation()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

    QDateTime lastUpdateTime = Qp::updateTimeInDatabase(parent);

    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(Qp::primaryKey(parent), OneToMany));

    for(int i = 0; i < childInts().size(); ++i) {
        qDebug() << "Testing child...";
        QTRY_VERIFY(lastUpdateTime < Qp::updateTimeInDatabase(parent));
        lastUpdateTime = Qp::updateTimeInDatabase(parent);

        Qp::SynchronizeResult result = Qp::synchronize(parent);
        QCOMPARE(result, Qp::Updated);

        QList<QSharedPointer<ChildObject> > children = parent->childObjectsOneToMany();
        for(int i2 = 0; i2 < childInts().size(); ++i2) {
            QSharedPointer<ChildObject> child = children.at(i2 + childInts().size() * i);
            QTRY_VERIFY(lastUpdateTime == Qp::updateTimeInDatabase(child));

            Qp::synchronize(child);
            QCOMPARE(child->someInt(), childInts().at(i2));
        }
    }
}

void SynchronizeTest::testSynchronizeManyToManyRelation()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();

    QDateTime lastUpdateTime = Qp::updateTimeInDatabase(parent);

    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(Qp::primaryKey(parent), ManyToMany));

    for(int i = 0; i < childInts().size(); ++i) {
        qDebug() << "Testing child...";
        QTRY_VERIFY(lastUpdateTime < Qp::updateTimeInDatabase(parent));
        lastUpdateTime = Qp::updateTimeInDatabase(parent);

        Qp::SynchronizeResult result = Qp::synchronize(parent);
        QCOMPARE(result, Qp::Updated);

        QList<QSharedPointer<ChildObject> > children = parent->childObjectsManyToMany();
        for(int i2 = 0; i2 < childInts().size(); ++i2) {
            QSharedPointer<ChildObject> child = children.at(i2 + childInts().size() * i);
            QTRY_VERIFY(lastUpdateTime == Qp::updateTimeInDatabase(child));

            Qp::synchronize(child);
            QCOMPARE(child->someInt(), childInts().at(i2));
        }
    }
}

void SynchronizeTest::testUpdateConflict()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QDateTime lastUpdateTime = Qp::updateTimeInDatabase(parent);

    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(Qp::primaryKey(parent), ChangeOnce));
    QTRY_VERIFY(lastUpdateTime < Qp::updateTimeInDatabase(parent));

    QCOMPARE(lastUpdateTime, Qp::updateTimeInObject(parent));

    parent->increaseCounter();
    Qp::UpdateResult result = Qp::update(parent);
    QCOMPARE(result, Qp::UpdateConflict);
}

void SynchronizeTest::testSynchronizeToSolveConflict()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QDateTime lastUpdateTime = Qp::updateTimeInDatabase(parent);

    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(Qp::primaryKey(parent), ChangeOnce));
    QTRY_VERIFY(lastUpdateTime < Qp::updateTimeInDatabase(parent));

    parent->increaseCounter();
    QCOMPARE(Qp::update(parent), Qp::UpdateConflict);

    Qp::synchronize(parent);
    QCOMPARE(Qp::updateTimeInDatabase(parent), Qp::updateTimeInObject(parent));

    parent->increaseCounter();
    Qp::UpdateResult result = Qp::update(parent);
    QCOMPARE(result, Qp::UpdateSuccess);
}

void SynchronizeTest::testCreatedSince()
{
    int count = 20;
    QDateTime now = Qp::databaseTime();

    QTest::qSleep(1000);
    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(count, CreateAndUpdate));

    QList<QSharedPointer<ParentObject>> result;

    QTRY_VERIFY((result = Qp::createdSince<ParentObject>(now)).size() == count);

    now = Qp::creationTimeInDatabase(result.last());

    QTRY_VERIFY((result = Qp::createdSince<ParentObject>(now)).size() == count);
    QCOMPARE(result.size(), count);
}

void SynchronizeTest::testUpdatedSince()
{
    int count = 20;
    QDateTime now = Qp::databaseTime();

    QTest::qSleep(1000);
    QScopedPointer<QProcess, SynchronizeTest> process(startChangerProcess(count, CreateAndUpdate));

    QList<QSharedPointer<ParentObject>> result;

    QTRY_VERIFY((result = Qp::createdSince<ParentObject>(now)).size() == count * 2);
    now = Qp::creationTimeInDatabase(result.last());
    QTRY_VERIFY((result = Qp::updatedSince<ParentObject>(now)).size() == count * 2);
}

void SynchronizeTest::startProcess()
{
    if(m_currentProcess)
        m_currentProcess->start();
}

QProcess *SynchronizeTest::startChangerProcess(int id, ChangerMode mode)
{
    m_currentProcess = new QProcess(this);
    m_currentProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    m_currentProcess->setProgram("../testDatabaseChanger/qpersistencetestdatabasechanger");
    m_currentProcess->setArguments(QStringList()
                                   << QString("%1").arg(id)
                                   << QString("%1").arg(mode));

    m_currentProcess->start();
    return m_currentProcess;
}
