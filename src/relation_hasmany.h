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
    void setObjects(const QList<QSharedPointer<QObject> > objects) const;

protected:
    virtual QVariant objectVariant(QSharedPointer<QObject> objects) const = 0;

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

    void append(QSharedPointer<T> object) {}
    void remove(QSharedPointer<T> object) {}

    operator QList<QSharedPointer<T>> () const { return Qp::castList<T>(objects()); }
    QpHasMany &operator=(const QList<QSharedPointer<T>> objects) { setObjects(Qp::castList<QObject>(objects)); return *this; }

protected:
    QVariant objectVariant(QSharedPointer<QObject> object) const
    {
        return QVariant::fromValue<QSharedPointer<T>>(qSharedPointerCast<T>(object));
    }

    Q_DISABLE_COPY(QpHasMany)
};

#ifndef QpRelation
#define QpRelation(Address) \
    "" #Address, this
#endif

#endif // RELATION_HASMANY_H
