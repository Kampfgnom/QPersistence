#ifndef QPERSISTENCE_ABSTRACTDATAACCESSOBJECT_H
#define QPERSISTENCE_ABSTRACTDATAACCESSOBJECT_H

#include <QtCore/QObject>

#include <QtCore/QSharedDataPointer>

uint qHash(const QVariant & var);

class QPersistenceError;
class QPersistenceMetaObject;

class QPersistenceAbstractDataAccessObjectPrivate;
class QPersistenceAbstractDataAccessObject : public QObject
{
    Q_OBJECT
public:
    ~QPersistenceAbstractDataAccessObject();

    virtual QPersistenceMetaObject dataSuiteMetaObject() const = 0;

    virtual int count() const = 0;
    virtual QList<QVariant> allKeys() const = 0;
    virtual QList<QObject *> readAllObjects() const = 0;
    virtual QObject *createObject() const = 0;
    virtual QObject *readObject(const QVariant &key) const = 0;
    virtual bool insertObject(QObject *object) = 0;
    virtual bool updateObject(QObject *object) = 0;
    virtual bool removeObject(QObject *object) = 0;

    QPersistenceError lastError() const;

Q_SIGNALS:
    void objectInserted(QObject *);
    void objectUpdated(QObject *);
    void objectRemoved(QObject *);

protected:
    explicit QPersistenceAbstractDataAccessObject(QObject *parent = 0);
    void setLastError(const QPersistenceError &error) const;
    void resetLastError() const;

private:
    Q_DISABLE_COPY(QPersistenceAbstractDataAccessObject)
    QSharedDataPointer<QPersistenceAbstractDataAccessObjectPrivate> d;
};

#endif // QPERSISTENCE_ABSTRACTDATAACCESSOBJECT_H
