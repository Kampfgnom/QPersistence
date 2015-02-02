#include "datasourceresult.h"
#include "reply.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpReplyData : public QSharedData
{
public:
    QpReplyData() : QSharedData(), result(nullptr), finished(false) {
    }

    QpDatasourceResult *result;
    QList<QSharedPointer<QObject> > objects;
    bool finished;
};

QpReply::QpReply(QpDatasourceResult *result, QObject *parent) : QObject(parent),
    data(new QpReplyData)
{
    setInternalResult(result);
}

QpReply::~QpReply()
{
}

QList<QSharedPointer<QObject> > QpReply::objects() const
{
    return data->objects;
}

bool QpReply::isFinished() const
{
    return data->finished;
}

void QpReply::finish()
{
    if(data->finished)
        return;

    data->finished = true;
    emit finished();
}

QpDatasourceResult *QpReply::internalResult() const
{
    return data->result;
}

void QpReply::setObjects(const QList<QSharedPointer<QObject> > &objects)
{
    data->objects = objects;
}

void QpReply::setInternalResult(QpDatasourceResult *result)
{
    data->result = result;
}
