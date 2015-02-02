#ifndef QPERSISTENCE_RELATION_HASMANY_H
#define QPERSISTENCE_RELATION_HASMANY_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "qpersistence.h"

class QpHasManyData;

class QpHasManyBase
{
public:
    QpHasManyBase(const QString &name, QObject *parent);
    ~QpHasManyBase();

    QList<QSharedPointer<QObject> > objects() const;
    void add(QSharedPointer<QObject> object);
    void remove(QSharedPointer<QObject> object);
    void setObjects(const QList<QSharedPointer<QObject> > objects) const;

    bool isResolved() const;
    bool operator ==(const QList<QSharedPointer<QObject> > &objects) const;

private:
    QExplicitlySharedDataPointer<QpHasManyData> data;
    Q_DISABLE_COPY(QpHasManyBase)
};

template<class T>
class QpHasMany : public QpHasManyBase
{
public:
    explicit QpHasMany(const QString &name, QObject *parent) : QpHasManyBase(name, parent) {
    }
    virtual ~QpHasMany() {
    }

    void add(QSharedPointer<T> object) {
        QpHasManyBase::add(qSharedPointerCast<QObject>(object));
    }
    void remove(QSharedPointer<T> object) {
        QpHasManyBase::remove(qSharedPointerCast<QObject>(object));
    }

    operator QList<QSharedPointer<T> > () const { return Qp::castList<T>(QpHasManyBase::objects()); }
    QpHasMany &operator=(const QList<QSharedPointer<T> > objects) {
        QpHasManyBase::setObjects(Qp::castList<QObject>(objects)); return *this;
    }
    bool operator ==(const QList<QSharedPointer<T> > &objects) const {
        return QpHasManyBase::operator ==(Qp::castList<QObject>(objects));
    }
};

#endif // QPERSISTENCE_RELATION_HASMANY_H
