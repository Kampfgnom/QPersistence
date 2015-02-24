#ifndef QPERSISTENCE_PROPERTYDEPENDENCIESHELPER_H
#define QPERSISTENCE_PROPERTYDEPENDENCIESHELPER_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QExplicitlySharedDataPointer>
#include <QSharedPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpMetaProperty;
class QpStorage;

class QpPropertyDependenciesData;
class QpPropertyDependenciesHelper
{
public:
    QpPropertyDependenciesHelper(QpStorage *storage);
    ~QpPropertyDependenciesHelper();
    QpPropertyDependenciesHelper(const QpPropertyDependenciesHelper &other);
    QpPropertyDependenciesHelper &operator = (const QpPropertyDependenciesHelper &other);

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

#endif // QPPROPERTYDEPENDENCIESHELPER_H
