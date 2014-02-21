#ifndef RELATION_HASMANY_H
#define RELATION_HASMANY_H

#include <QExplicitlySharedDataPointer>
#include <QSharedPointer>
#include <QVariant>

#include "qpersistence.h"

class QpHasManyData;

class QpHasManyBase
{
public:
    QpHasManyBase(const QString &name, QObject *parent);
    ~QpHasManyBase();

    QList<QSharedPointer<QObject> > objects() const;
    void append(QSharedPointer<QObject> object);
    void remove(QSharedPointer<QObject> object);
    void setObjects(const QList<QSharedPointer<QObject> > objects) const;

private:
    QExplicitlySharedDataPointer<QpHasManyData> data;
    Q_DISABLE_COPY(QpHasManyBase)
};

template<class T>
class QpHasMany : public QpHasManyBase
{
public:
    explicit QpHasMany(const QString &name, QObject *parent) : QpHasManyBase(name, parent) {}
    virtual ~QpHasMany() {}

    void append(QSharedPointer<T> object) { QpHasManyBase::append(qSharedPointerCast<QObject>(object)); }
    void remove(QSharedPointer<T> object) { QpHasManyBase::remove(qSharedPointerCast<QObject>(object)); }

    operator QList<QSharedPointer<T> > () const { return Qp::castList<T>(QpHasManyBase::objects()); }
    QpHasMany &operator=(const QList<QSharedPointer<T> > objects) { QpHasManyBase::setObjects(Qp::castList<QObject>(objects)); return *this; }
};

#endif // RELATION_HASMANY_H
