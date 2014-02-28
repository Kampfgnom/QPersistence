#ifndef TESTS_COMMON_H
#define TESTS_COMMON_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QString>
#include <QtTest>
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "childobject.h"
#include "parentobject.h"
#include "../src/sqlbackend.h"
#include "../src/sqlquery.h"
#include <QPersistence.h>

QVariant NULLKEY();

#endif // TESTS_COMMON_H
