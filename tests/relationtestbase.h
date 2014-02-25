#ifndef RELATIONTESTBASE_H
#define RELATIONTESTBASE_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QtTest>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

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
