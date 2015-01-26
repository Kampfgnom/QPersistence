#ifndef QPPROPERTYDEPENDENCIES_H
#define QPPROPERTYDEPENDENCIES_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QExplicitlySharedDataPointer>
#include <QSharedPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpMetaProperty;
class QpStorage;

class QpPropertyDependenciesData;
class QpPropertyDependencies
{
public:
    QpPropertyDependencies();
    QpPropertyDependencies(QpStorage *storage);
    ~QpPropertyDependencies();
    QpPropertyDependencies(const QpPropertyDependencies &other);
    QpPropertyDependencies &operator = (const QpPropertyDependencies &other);

    void initSelfDependencies(QSharedPointer<QObject> object) const;
    void initDependencies(QObject *object,
                          QList<QSharedPointer<QObject> > relatedObjects,
                          const QpMetaProperty &relation) const;
    void initDependencies(QObject *object,
                          QSharedPointer<QObject> related,
                          const QpMetaProperty &relation) const;
    void removeDependencies(QObject *object,
                            QSharedPointer<QObject> related,
                            const QpMetaProperty &relation) const;

private:
    QExplicitlySharedDataPointer<QpPropertyDependenciesData> data;
};

#endif // QPPROPERTYDEPENDENCIES_H
