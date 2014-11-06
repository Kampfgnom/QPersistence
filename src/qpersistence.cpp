#include "qpersistence.h"

namespace Qp {

int variantUserType(const QMetaObject &metaObject)
{
    return Qp::Private::variantCast(QSharedPointer<QObject>(), metaObject.className()).userType();
}

}
