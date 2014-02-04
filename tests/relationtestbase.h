#ifndef RELATIONTESTBASE_H
#define RELATIONTESTBASE_H

#include <QObject>
#include <QtTest>
#include <QPersistence.h>
#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlquery.h"
#include "../src/sqlcondition.h"

class RelationTestBase : public QObject
{
    Q_OBJECT
public:
    explicit RelationTestBase(QObject *parent = 0);

    void VERIFY_QP_ERROR();

protected:
    void initDatabase();
};

#endif // RELATIONTESTBASE_H
