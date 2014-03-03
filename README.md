QPersistence 0.0
================

QPersistence is a set of Qt classes for persisting QObjects.

The library is currently only meant for internal usage. Some day it might be good enough for usage in other projects as well, but at the moment I would refrain from cloning it :wink:

Getting started
===============

QPersistence stores QObjects in QSqlDatabases. You have to establish the connection to the database register your model classes and make shure, that the database schema matches your model. QPersistence assists you with creating your schema with two methods:

```` C++
QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
db.setDatabaseName("testdb.sqlite");
db.open();
Qp::registerClass<Object>();
Qp::setDatabase(db);
Qp::createCleanSchema(); // DROPS ALL TABLES!
// or
// Qp::adjustSchema();
````

Note that only SQLite and MySQL databases are currently supported and that you have to compile QPersistence with either `QP_FOR_MYSQL` or `QP_FOR_SQLITE` defined (since SQLite does not support all features).

Models
------

QPersistence makes heavy usage of Qt’s meta object system. Specifying a model class is as easy as following the guidelines from the [Qt Documentation](http://qt-project.org/doc/qt-5/properties.html):

```` C++
#include <QPersistence.h>

class Object : public QObject
{
	Q_OBJECT
    Q_PROPERTY(QString aString READ aString WRITE setAString)

public:
    explicit ParentObject(QObject *parent = 0);
    ~ParentObject();

    QString aString() const;
    void setAString(const QString &value);

private:
	QString m_aString;
};

````

You can also use `enums` or even `QFlags` as property types. These will also be mapped to MySQL `ENUM` and `SET` accordingly.

When you have registered the class and established the database connection you may create, update, remove and read objects:

```` C++
QSharedPointer<Object> object = Qp::create<Object>();
object->setAString("myString");
Qp::update(object);

QList<QSharedPointer<Object>> list = Qp::readAll<Object>();

int id = Qp::primaryKey(list.last()); // internal primary key

QSharedPointer<Object> lastObject = Qp::read<Object>(123); // lastObject == list.last()
Qp::remove(lastObject);
````

QPersistence manages the lifetime of each object for you, by exposing objects only inside QSharedPointers.

Relations
---------

QPersistence assists you in creating and managing relations between objects, first by again using the meta property system, and second by giving you four helper classes, which keep each relation in sync between objects.

You register the relations with the meta property system as follows:

```` C++
#include <QPersistence.h>

class ParentObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSharedPointer<ChildObject> hasOneChild READ hasOneChild WRITE setOneChild)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:hasOneChild",
                "reverserelation=belongsToOneParent")
                
public:
    QSharedPointer<ChildObject> hasOneChild() const {
    	return m_hasOne;
    }
    
public slots:
    void setOneChild(QSharedPointer<ChildObject> child) {
    	m_hasOne = child;
    }
    
private:
    QpHasOne<ChildObject> m_hasOne;
};
````
Note that you have to specify the reverse relation, because there might be several relations between two classes.

Also note, how the `QpHasOne<ChildObject>` class gives you implicit casts to and from `QSharedPointer<ChildObject>`. The other relation classes are `QpHasMany`, `QpBelongsToOne` and `QpBelongsToMany`. On each 'side' of a relation there must be one "has-a" and one "belongs-to" relation. The "to-many" relations implicitly cast to `QList<QSharedPointer<Object>>`.

_Important_:  The "to-many" additionally have an `add` and `remove` method, which you _have to_ expose in slots named `add|removeProperty(QSharedPointer<Object)`, so that QPersistence can adjust the relations.

These methods are used to keep the reverse relation in sync. Take for example a hasMany-belongsToOne relation: If you add an object to the hasMany relation, it will automatically be removed from the object it belonged to before and the belongsToOne relation points to the new owner.

Debugging
---------

QPeristence exposes all errors via the `QpError Qp::lastError()` interface. You should test the last error for validity after each operation. 

If something odd goes wrong, you might want to enable debug output by calling `Qp::setSqlDebugEnabled(true)`. You will then see all SQL queries which QPersistence uses internally.


Synchronizing
-------------

For MySQL databases each object stores its creation and last update time. You can query these times with `Qp::creationTimeInDatabase()` and `updateTimeInDatabase()`. The current database time is `databaseTime()`.

These times can be used to synchronize an object between several instances of your application. With `Qp::synchronize(object)` you can keep the object synchronized with the database. Note that all local changes to the object will be overwritten by the changes in the database, including relations.

You can use `Qp::updatedSince(dateTime)` and `Qp::createdSince(dateTime` to get all updated/created objects. Note that the `updatedSince` is a real superset of `createdSince`, because for new objects the update time equals its creation time.

If you for example wanted to display incoming items you could do the following:

```` C++
void Window::myUpdateSlot() {
	QList<QSharedPointer<Object>> list = Qp::createdSince(m_lastSyncTime);
	// display list
	if(!list.isEmpty)
		m_lastSyncTime = list.last();
		
	// start timer for slot again
}
````

Locks
-----

QpLock is a class which helps you to safely attach information to an object, which you can use to not run into synchronization conflicts.

Say you wanted to show a dialog for editing an object. You would first try to lock the object, to prevent several instances of the app to modify the same object:

```` C++
QpLock lock = Qp::tryLock(object);
if(lock.status() == QpLock::LockedRemotely) {
	// show error dialog
	return;
}

// show edit dialog
// update object

Qp::unlock(object);
````

Note that while QpLock is implemented in a way where updates to the object are blocking _while creating_ the lock, nothing prevents you from updating objects, while someone holds a lock for it. You have to manually check for locks yourself.

If you want to query the lock-status of a row without locking it, you can use `QpLock Qp::isLocked()`.

You can also store additional information with each lock. Simply pass a `QHash<String, QVariant>` to `tryLock`, which other instances of your app can read from `QpLock::additionalInformation(QString)`.

Before using locks you have to enable locks on the database. Do this before adjusting/creating your schema, because locks require a column in each table and their own row. Each additional information field, which you plan to store inside the `QHash` also has to be registered beforehand:

```` C++
Qp::addAdditionalLockInformationField("myField", QVariant::Int);
Qp::enableLocks();
// register classes...
Qp::adjustSchema();
````


License
=======

QPersistence is licensed under the LGPLv3.0. See LICENSE for details.
