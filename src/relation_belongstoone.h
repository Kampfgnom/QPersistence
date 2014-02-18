#ifndef RELATION_BELONGSTOONE_H
#define RELATION_BELONGSTOONE_H

#include <QExplicitlySharedDataPointer>
#include <QSharedPointer>
#include <QVariant>

class QpBelongsToOneData;

class QpBelongsToOneBase
{
public:
    QpBelongsToOneBase(const QString &name, QObject *parent);
    virtual ~QpBelongsToOneBase();

    QSharedPointer<QObject> object() const;
    void setObject(const QSharedPointer<QObject> object) const;

protected:
    virtual QVariant objectVariant(QSharedPointer<QObject> object) const = 0;

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

protected:
    QVariant objectVariant(QSharedPointer<QObject> object) const
    {
        return QVariant::fromValue<QSharedPointer<T>>(qSharedPointerCast<T>(object));
    }
};

#ifndef QpRelation
#define QpRelation(Address) \
    "" ## Address, this
#endif

#endif // RELATION_BELONGSTOONE_H
