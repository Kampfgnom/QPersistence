#include "tst_relationsindatabasetest.h"

#include <QPersistence.h>
#include <QSqlQuery>
#include <QSqlError>
#include "parentobject.h"
#include "childobject.h"

RelationsInDatabaseTest::RelationsInDatabaseTest(QObject *parent) :
    QObject(parent)
{
}

void RelationsInDatabaseTest::initTestCase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("192.168.100.2");
    db.setDatabaseName("niklas");
    db.setUserName("niklas");
    db.setPassword("niklas");

    QVERIFY2(db.open(), db.lastError().text().toUtf8());

    Qp::setSqlDebugEnabled(true);
    Qp::setDatabase(db);
    Qp::registerClass<ParentObject>();
    Qp::registerClass<ChildObject>();
    Qp::createCleanSchema();

    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}

void RelationsInDatabaseTest::testSetOneToOneFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    QVERIFY(fkInDatabase(parent, "childObject").size() == 0);

    QSqlQuery q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0), QVariant(QVariant().toInt()));

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    q = fkInDatabase(parent, "childObject");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(child));

    q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(parent));
}

void RelationsInDatabaseTest::testSetOneToOneFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    QVERIFY(fkInDatabase(parent, "childObject").size() == 0);

    QSqlQuery q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0), QVariant(QVariant().toInt()));

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    q = fkInDatabase(parent, "childObject");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(child));

    q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(parent));
}

void RelationsInDatabaseTest::testSetAnotherOneToOneFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    QSharedPointer<ChildObject> child2 = Qp::create<ChildObject>();
    parent->setChildObjectOneToOne(child2);
    Qp::update(parent);

    QSqlQuery q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0), QVariant(QVariant().toInt()));

    q = fkInDatabase(parent, "childObject");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(child2));

    q = fkInDatabase(child2, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(parent));
}

void RelationsInDatabaseTest::testSetAnotherOneToOneFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    QSharedPointer<ChildObject> child2 = Qp::create<ChildObject>();
    parent->setChildObjectOneToOne(child2);
    Qp::update(child2);

    QSqlQuery q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0), QVariant(QVariant().toInt()));

    q = fkInDatabase(parent, "childObject");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(child2));

    q = fkInDatabase(child2, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0).toInt(), Qp::primaryKey(parent));
}

void RelationsInDatabaseTest::testClearOneToOneRelationFromParent()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(parent);

    parent->setChildObjectOneToOne(QSharedPointer<ChildObject>());
    Qp::update(parent);

    QSqlQuery q = fkInDatabase(parent, "childObject");
    QCOMPARE(q.size(), 0);

    q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0), QVariant(QVariant().toInt()));
}

void RelationsInDatabaseTest::testClearOneToOneRelationFromChild()
{
    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    QSharedPointer<ChildObject> child = Qp::create<ChildObject>();

    parent->setChildObjectOneToOne(child);
    Qp::update(child);

    parent->setChildObjectOneToOne(QSharedPointer<ChildObject>());
    Qp::update(child);

    QSqlQuery q = fkInDatabase(parent, "childObject");
    QCOMPARE(q.size(), 0);

    q = fkInDatabase(child, "parentObjectOneToOne");
    QVERIFY(q.first());
    QVERIFY(q.size() == 1);
    QCOMPARE(q.value(0), QVariant(QVariant().toInt()));

}

void RelationsInDatabaseTest::testSetOneToManyFromParent()
{
    const int CHILDCOUNT = 4;

    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
    }
    Qp::update(parent);

    QSqlQuery q = fkInDatabase(parent, "childObjects");
    QVariantList fks;
    while(q.next()) {
        fks.append(q.value(0).toInt());
    }

    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        QSqlQuery q2 = fkInDatabase(child, "parentObjectOneToMany");
        QVERIFY(q2.first());
        QVERIFY(q2.size() == 1);
        QCOMPARE(q2.value(0).toInt(), Qp::primaryKey(parent));

        QVERIFY(fks.contains(Qp::primaryKey(child)));
        fks.removeAll(Qp::primaryKey(child));
    }
    QVERIFY(fks.isEmpty());
}

void RelationsInDatabaseTest::testSetOneToManyFromChild()
{
    const int CHILDCOUNT = 4;

    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        Qp::update(child);
    }

    QSqlQuery q = fkInDatabase(parent, "childObjects");
    QVariantList fks;
    while(q.next()) {
        fks.append(q.value(0).toInt());
    }

    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        QSqlQuery q2 = fkInDatabase(child, "parentObjectOneToMany");
        QVERIFY(q2.first());
        QVERIFY(q2.size() == 1);
        QCOMPARE(q2.value(0).toInt(), Qp::primaryKey(parent));

        QVERIFY(fks.contains(Qp::primaryKey(child)));
        fks.removeAll(Qp::primaryKey(child));
    }
    QVERIFY(fks.isEmpty());
}

void RelationsInDatabaseTest::testSetAnotherOneToManyFromParent()
{
    const int CHILDCOUNT = 4;

    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
    }
    Qp::update(parent);

    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        parent2->addChildObjectOneToMany(child);
    }
    Qp::update(parent2);

    QSqlQuery q = fkInDatabase(parent, "childObjects");
    QCOMPARE(q.size(), 0);

    q = fkInDatabase(parent2, "childObjects");
    QVariantList fks;
    while(q.next()) {
        fks.append(q.value(0).toInt());
    }

    foreach(QSharedPointer<ChildObject> child, parent2->childObjectsOneToMany()) {
        QSqlQuery q2 = fkInDatabase(child, "parentObjectOneToMany");
        QVERIFY(q2.first());
        QVERIFY(q2.size() == 1);
        QCOMPARE(q2.value(0).toInt(), Qp::primaryKey(parent2));

        QVERIFY(fks.contains(Qp::primaryKey(child)));
        fks.removeAll(Qp::primaryKey(child));
    }
    QVERIFY(fks.isEmpty());
}

void RelationsInDatabaseTest::testSetAnotherOneToManyFromChild()
{
    const int CHILDCOUNT = 4;

    QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
    for(int i = 0; i < CHILDCOUNT; ++i) {
        QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
        parent->addChildObjectOneToMany(child);
        Qp::update(child);
    }

    QSharedPointer<ParentObject> parent2 = Qp::create<ParentObject>();
    foreach(QSharedPointer<ChildObject> child, parent->childObjectsOneToMany()) {
        parent2->addChildObjectOneToMany(child);
        Qp::update(child);
    }

    QSqlQuery q = fkInDatabase(parent, "childObjects");
    QCOMPARE(q.size(), 0);

    q = fkInDatabase(parent2, "childObjects");
    QVariantList fks;
    while(q.next()) {
        fks.append(q.value(0).toInt());
    }

    foreach(QSharedPointer<ChildObject> child, parent2->childObjectsOneToMany()) {
        QSqlQuery q2 = fkInDatabase(child, "parentObjectOneToMany");
        QVERIFY(q2.first());
        QVERIFY(q2.size() == 1);
        QCOMPARE(q2.value(0).toInt(), Qp::primaryKey(parent2));

        QVERIFY(fks.contains(Qp::primaryKey(child)));
        fks.removeAll(Qp::primaryKey(child));
    }
    QVERIFY(fks.isEmpty());
}

void RelationsInDatabaseTest::testSetManyToManyRelationFromParent()
{
    const int CHILDCOUNT = 4;
    const int PARENTCOUNT = 3;

    QList<QSharedPointer<ParentObject>> parents;
    QList<QSharedPointer<ChildObject>> children;
    for(int j = 0; j < PARENTCOUNT; ++j) {
        QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
        foreach(QSharedPointer<ParentObject> parent2, parents) {
            foreach(QSharedPointer<ChildObject> child2, parent2->childObjectsOneToMany()) {
                parent->addChildObjectManyToMany(child2);
            }
        }

        for(int i = 0; i < CHILDCOUNT; ++i) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            children.append(child);
            parent->addChildObjectManyToMany(child);
        }

        parents.append(parent);
        Qp::update(parent);
    }

    foreach(QSharedPointer<ParentObject> parent, parents) {
        QSqlQuery q = fkInDatabase(parent, "childObjectsManyToMany");
        QVariantList fks;
        while(q.next()) {
            fks.append(q.value(0).toInt());
        }
        QCOMPARE(fks.size(), parent->childObjectsManyToMany().size());

        foreach(QSharedPointer<ChildObject> child, parent->childObjectsManyToMany()) {
            QVERIFY(fks.contains(Qp::primaryKey(child)));
            fks.removeAll(Qp::primaryKey(child));
        }
        QVERIFY(fks.isEmpty());
    }

    foreach(QSharedPointer<ChildObject> child, children) {
        QSqlQuery q = fkInDatabase(child, "parentObjectsManyToMany");
        QVariantList fks;
        while(q.next()) {
            fks.append(q.value(0).toInt());
        }
        QCOMPARE(fks.size(), child->parentObjectsManyToMany().size());

        foreach(QSharedPointer<ParentObject> parent, child->parentObjectsManyToMany()) {
            QVERIFY(fks.contains(Qp::primaryKey(parent)));
            fks.removeAll(Qp::primaryKey(parent));
        }
        QVERIFY(fks.isEmpty());
    }
}

void RelationsInDatabaseTest::testSetManyToManyRelationFromChild()
{
    const int CHILDCOUNT = 4;
    const int PARENTCOUNT = 3;

    QList<QSharedPointer<ParentObject>> parents;
    QList<QSharedPointer<ChildObject>> children;
    for(int j = 0; j < PARENTCOUNT; ++j) {
        QSharedPointer<ParentObject> parent = Qp::create<ParentObject>();
        foreach(QSharedPointer<ParentObject> parent2, parents) {
            foreach(QSharedPointer<ChildObject> child2, parent2->childObjectsOneToMany()) {
                parent->addChildObjectManyToMany(child2);
                Qp::update(child2);
            }
        }

        for(int i = 0; i < CHILDCOUNT; ++i) {
            QSharedPointer<ChildObject> child = Qp::create<ChildObject>();
            children.append(child);
            parent->addChildObjectManyToMany(child);
            Qp::update(child);
        }

        parents.append(parent);
    }

    foreach(QSharedPointer<ParentObject> parent, parents) {
        QSqlQuery q = fkInDatabase(parent, "childObjectsManyToMany");
        QVariantList fks;
        while(q.next()) {
            fks.append(q.value(0).toInt());
        }
        QCOMPARE(fks.size(), parent->childObjectsManyToMany().size());

        foreach(QSharedPointer<ChildObject> child, parent->childObjectsManyToMany()) {
            QVERIFY(fks.contains(Qp::primaryKey(child)));
            fks.removeAll(Qp::primaryKey(child));
        }
        QVERIFY(fks.isEmpty());
    }

    foreach(QSharedPointer<ChildObject> child, children) {
        QSqlQuery q = fkInDatabase(child, "parentObjectsManyToMany");
        QVariantList fks;
        while(q.next()) {
            fks.append(q.value(0).toInt());
        }
        QCOMPARE(fks.size(), child->parentObjectsManyToMany().size());

        foreach(QSharedPointer<ParentObject> parent, child->parentObjectsManyToMany()) {
            QVERIFY(fks.contains(Qp::primaryKey(parent)));
            fks.removeAll(Qp::primaryKey(parent));
        }
        QVERIFY(fks.isEmpty());
    }
}


void RelationsInDatabaseTest::VERIFY_QP_ERROR()
{
    QVERIFY2(!Qp::lastError().isValid(), Qp::lastError().text().toUtf8());
}
