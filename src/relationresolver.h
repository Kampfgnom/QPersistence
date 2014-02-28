#ifndef QPERSISTENCE_RELATIONRESOLVER_H
#define QPERSISTENCE_RELATIONRESOLVER_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpMetaProperty;

class QpRelationResolver
{
public:
    static void readRelationFromDatabase(const QpMetaProperty &relation, QObject *object);
    static QList<QSharedPointer<QObject > > resolveRelation(const QString &name, const QObject *object);
    static QSharedPointer<QObject> resolveToOneRelation(const QString &name, const QObject *object);
    static QList<QSharedPointer<QObject > > resolveToManyRelation(const QString &name, const QObject *object);
};

#endif // QPERSISTENCE_RELATIONRESOLVER_H
