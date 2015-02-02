#ifndef QPREPLY_H
#define QPREPLY_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpError;
class QpDatasourceResult;

class QpReplyData;
class QpReply : public QObject
{
    Q_OBJECT
public:
    ~QpReply();

    QList<QSharedPointer<QObject> > objects() const;

    bool isFinished() const;

signals:
    void finished();

private slots:
    friend class QpDataAccessObjectBase;

    void finish();
    void setObjects(const QList<QSharedPointer<QObject> > &objects);
    void setInternalResult(QpDatasourceResult *result);

private:
    explicit QpReply(QpDatasourceResult *result, QObject *parent = 0);

    QSharedDataPointer<QpReplyData> data;

    QpDatasourceResult *internalResult() const;
};

#endif // QPREPLY_H
