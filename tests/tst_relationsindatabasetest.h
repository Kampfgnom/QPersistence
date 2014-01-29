#ifndef TST_RELATIONSINDATABASETEST_H
#define TST_RELATIONSINDATABASETEST_H

#include <QObject>
#include <QtTest>

#include <QPersistence.h>
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"
#include <QSqlError>

class RelationsInDatabaseTest : public QObject
{
    Q_OBJECT
public:
    explicit RelationsInDatabaseTest(QObject *parent = 0);

    void VERIFY_QP_ERROR();

    template<class T>
    QSqlQuery fkInDatabase(QSharedPointer<T> object, const QString &relationName);

private Q_SLOTS:
    void initTestCase();

    void testSetOneToOneFromParent();
    void testSetOneToOneFromChild();
    void testSetAnotherOneToOneFromParent();
    void testSetAnotherOneToOneFromChild();
    void testClearOneToOneRelationFromParent();
    void testClearOneToOneRelationFromChild();

    void testSetOneToManyFromParent();
    void testSetOneToManyFromChild();
    void testSetAnotherOneToManyFromParent();
    void testSetAnotherOneToManyFromChild();

    void testSetManyToManyRelationFromParent();
    void testSetManyToManyRelationFromChild();
};

template<class T>
QSqlQuery RelationsInDatabaseTest::fkInDatabase(QSharedPointer<T> object, const QString &relationName)
{
    QpMetaObject metaObject = QpMetaObject::forClassName(T::staticMetaObject.className());
    QpMetaProperty relation = metaObject.metaProperty(relationName);
    QVariant primaryKey = Qp::primaryKey(object);

    if(relation.cardinality() == QpMetaProperty::OneToOneCardinality) {
        if(relation.hasTableForeignKey()) {
            QpSqlQuery select(Qp::database());
            select.setTable(relation.tableName());
            select.addField(relation.columnName());
            select.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                    QpSqlCondition::EqualTo,
                                                    primaryKey));
            select.prepareSelect();
            select.exec();
            return select;
        }

        QpSqlQuery select(Qp::database());
        select.setTable(relation.tableName());
        select.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
        select.setWhereCondition(QpSqlCondition(relation.columnName(),
                                                QpSqlCondition::EqualTo,
                                                primaryKey));
        select.prepareSelect();
        select.exec();
        return select;
    }

    if(relation.cardinality() == QpMetaProperty::OneToManyCardinality) {
        QpSqlQuery select(Qp::database());
        select.setTable(relation.tableName());
        select.addField(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY);
        select.setWhereCondition(QpSqlCondition(relation.columnName(),
                                                QpSqlCondition::EqualTo,
                                                primaryKey));
        select.prepareSelect();
        select.exec();
        return select;
    }


    if(relation.cardinality() == QpMetaProperty::ManyToOneCardinality) {
        QpSqlQuery select(Qp::database());
        select.setTable(relation.tableName());
        select.addField(relation.columnName());
        select.setWhereCondition(QpSqlCondition(QpDatabaseSchema::COLUMN_NAME_PRIMARY_KEY,
                                                QpSqlCondition::EqualTo,
                                                primaryKey));
        select.prepareSelect();
        select.exec();
        return select;
    }

    if(relation.cardinality() == QpMetaProperty::ManyToManyCardinality) {
        QpSqlQuery select(Qp::database());
        select.setTable(relation.tableName());
        select.addField(relation.reverseRelation().columnName());
        select.setWhereCondition(QpSqlCondition(relation.columnName(),
                                                QpSqlCondition::EqualTo,
                                                primaryKey));
        select.prepareSelect();
        select.exec();
        return select;
    }

    Q_ASSERT(false);
    return QpSqlQuery();
}

#endif // TST_RELATIONSINDATABASETEST_H
