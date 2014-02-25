#ifndef QPERSISTENCE_RELATION_BELONGSTOONE_H
#define QPERSISTENCE_RELATION_BELONGSTOONE_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QMetaMethod>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpBelongsToOneData;
class QpBelongsToOneBase
{
public:
    QpBelongsToOneBase(const QString &name, QObject *parent);
    virtual ~QpBelongsToOneBase();

    QSharedPointer<QObject> object() const;
    void setObject(const QSharedPointer<QObject> object) const;

private:
    QExplicitlySharedDataPointer<QpBelongsToOneData> data;
};

template<class T>
class QpBelongsToOne : public QpBelongsToOneBase
{
public:
    explicit QpBelongsToOne(const QString &name, QObject *parent) : QpBelongsToOneBase(name, parent) {}
    virtual ~QpBelongsToOne() {}

    operator QSharedPointer<T> () const { return qSharedPointerCast<T>(object()); }
    QpBelongsToOne &operator=(const QSharedPointer<T> object) { setObject(qSharedPointerCast<QObject>(object)); return *this; }
};

#endif // QPERSISTENCE_RELATION_BELONGSTOONE_H
