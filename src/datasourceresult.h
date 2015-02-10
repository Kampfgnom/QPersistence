#ifndef QPERSISTENCE_DATASOURCERESULT_H
#define QPERSISTENCE_DATASOURCERESULT_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QMetaProperty>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpError;
class QpDataAccessObjectBase;
class QpDataTransferObjectDiff;

class QpDataTransferObject
{
public:
    QpDataTransferObject();
    int primaryKey;
    QMetaObject metaObject;
    QHash<int, QVariant> properties;
    QHash<QString, QVariant> dynamicProperties;
    QHash<int, int> toOneRelationFKs;
    QHash<int, QList<int>> toManyRelationFKs;
    QHash<int, QList<int>> toManyRelationFKsAdded; //! Only used for comparisons
    QHash<int, QList<int>> toManyRelationFKsRemoved;//! Only used for comparisons

    int revision() const; //! the revision in this dto
    bool isEmpty() const; //! true if properties, dynamicProperties, toOneRelationFKs and toManyRelationFKs are empty

    QpDataTransferObjectDiff diff(const QObject *object) const; //! compares the changes of this DTO with the changes of the object
    QpDataTransferObject compare(const QpDataTransferObject &other) const; //! Returns a DTO containing all the changes which other has compared to this object

    /*!
     * \brief merge Merges with other
     * Other's value take precedence.
     * toMany relations will use left as base and right's toManyRelationFKsAdded and toManyRelationFKsRemoved to perform the merge
     */
    QpDataTransferObject merge(const QpDataTransferObject &other) const;

    void write(QObject *object) const;
    QpDataTransferObjectDiff rebase(QObject *object) const;
    static QpDataTransferObject fromObject(const QObject *object); //! The "_Qp_dataTransferObject" dynamic property
    static QpDataTransferObject readObject(const QObject *object); //! Reads all object properties
};

class QpDataTransferObjectDiff
{
public:
    QpDataTransferObject left; //! "in-object" changes
    QpDataTransferObject right; //! "remote" changes
    QpDataTransferObject conflictsLeft;
    QpDataTransferObject conflictsRight;

    bool isEmpty() const;
    bool isConflict() const; //! true if there are any conflicts
};

typedef QHash<int, QpDataTransferObject> QpDataTransferObjectsById;

class QpDatasourceResultData;
class QpDatasourceResult : public QObject
{
    Q_OBJECT
public:

    explicit QpDatasourceResult(const QpDataAccessObjectBase *dao = 0);
    ~QpDatasourceResult();

    int integerResult() const;
    int size() const;
    bool isEmpty() const;
    QList<QpDataTransferObject> dataTransferObjects() const;
    QpDataTransferObjectsById dataTransferObjectsById() const;

    bool isValid() const;
    QpError lastError() const;

public slots:
    void finish();
    void reset();
    void setIntegerResult(int result);
    void setDataTransferObjects(const QpDataTransferObjectsById &dataTransferObjects);
    void addDataTransferObject(const QpDataTransferObject &dataTransferObject);
    void setLastError(const QpError &lastError);

signals:
    void finished();
    void error(const QpError &error);

private:
    QExplicitlySharedDataPointer<QpDatasourceResultData> data;
};

Q_DECLARE_METATYPE(QpDataTransferObject)
Q_DECLARE_METATYPE(QpDataTransferObjectsById)

#endif // QPERSISTENCE_DATASOURCERESULT_H
