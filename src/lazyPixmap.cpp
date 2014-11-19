#include "lazyPixmap.h"

#ifdef QP_NO_GUI
void qpunused() {}
#else

#include "sqldataaccessobjecthelper.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QSharedData>
#include <QPixmap>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#include "storage.h"

class QpLazyPixmapData : public QSharedData {
    public:
        QString name;
        QPixmap pixmap;
        QObject *parent;

        void load();
};

void QpLazyPixmapData::load()
{
    if(Qp::Private::primaryKey(parent) == 0)
        return;

    QpStorage *storage = QpStorage::forObject(parent);
    QpSqlDataAccessObjectHelper *helper = storage->sqlDataAccessObjectHelper();
    storage->setSqlDebugEnabled(true);
    pixmap = helper->readPixmap(parent, name);
    storage->setSqlDebugEnabled(false);
}

QpLazyPixmap::QpLazyPixmap(const QString &name, QObject *parent) :
    data(new QpLazyPixmapData)
{
    int classNameEndIndex = name.lastIndexOf("::");
    QString n = name;
    if(classNameEndIndex >= 0)
        n = name.mid(classNameEndIndex + 2);
    data->name = n;
    data->parent = parent;
}

QpLazyPixmap::~QpLazyPixmap()
{
}

QPixmap QpLazyPixmap::pixmap() const
{
    return data->pixmap;
}

bool QpLazyPixmap::isNull() const
{
    return data->pixmap.isNull();
}

QpLazyPixmap &QpLazyPixmap::operator=(const QPixmap &pixmap)
{
    data->pixmap = pixmap;
    return *this;
}

QpLazyPixmap::operator QPixmap () const
{
    if(data->pixmap.isNull()) {
        data->load();
    }

    return data->pixmap;
}

#endif

