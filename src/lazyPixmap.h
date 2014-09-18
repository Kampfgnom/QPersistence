#ifndef LAZYPIXMAP_H
#define LAZYPIXMAP_H

#include "defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QExplicitlySharedDataPointer>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QObject;
class QPixmap;

class QpLazyPixmapData;
class QpLazyPixmap
{
    public:
        explicit QpLazyPixmap(const QString &name, QObject *parent);
        ~QpLazyPixmap();

        QPixmap pixmap() const;
        bool isNull() const;

        operator QPixmap () const;
        QpLazyPixmap &operator=(const QPixmap &objects);

    private:
        QExplicitlySharedDataPointer<QpLazyPixmapData> data;
};

#endif // LAZYPIXMAP_H
