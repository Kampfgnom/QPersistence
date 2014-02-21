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

protected:
    virtual QVariant objectVariant(QSharedPointer<QObject> objects) const = 0;
    virtual void invokeMethod(QMetaMethod method, QObject *object, QSharedPointer<QObject> arg) const = 0;

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

protected:
    QVariant objectVariant(QSharedPointer<QObject> object) const
    {
        return QVariant::fromValue<QSharedPointer<T>>(qSharedPointerCast<T>(object));
    }

    void invokeMethod(QMetaMethod method, QObject *object, QSharedPointer<QObject> arg) const Q_DECL_OVERRIDE
    {
        method.invoke(object, Q_ARG(QSharedPointer<T>, qSharedPointerCast<T>(arg)));
    }

    Q_DISABLE_COPY(QpHasMany)
};

#ifndef QpRelation
#define QpRelation(Address) \
    "" #Address, this
#endif

#endif // RELATION_HASMANY_H
