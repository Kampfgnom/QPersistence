#include "relation_hasmany.h"
#include <QSharedData>

#include "metaproperty.h"
#include "relationresolver.h"
#include "qpersistence.h"

class QpHasManyData : public QSharedData {
public:
    QpHasManyData() : QSharedData(),
        parent(nullptr)
    {}

    QSharedPointer<QObject> object;
    QpMetaProperty metaProperty;
    QObject *parent;
};

QpHasManyBase::QpHasManyBase(const QString &name, QObject *parent) :
    data(new QpHasManyData)
{
    data->parent = parent;
    QString n = name.mid(name.lastIndexOf("::") + 2);
    data->metaProperty = QpMetaObject::forObject(parent).metaProperty(n);
}

QpHasManyBase::~QpHasManyBase()
{
}

QList<QSharedPointer<QObject>> QpHasManyBase::objects() const
{

}

void QpHasManyBase::setObjects(const QList<QSharedPointer<QObject>> object) const
{

}
