#include "tests_common.h"

QVariant NULLKEY() {
#ifdef QP_FOR_MYSQL
    return QVariant(QVariant().toInt());
#elif defined QP_FOR_SQLITE
    return QVariant(QVariant().toString());
#endif
}
